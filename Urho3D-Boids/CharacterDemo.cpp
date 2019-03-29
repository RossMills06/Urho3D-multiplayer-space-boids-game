//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Terrain.h>
#include <string> 

#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/CheckBox.h>

#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#include "Character.h"
#include "CharacterDemo.h"
#include "Touch.h"
#include "boids.h"
#include "Missile.h"
#include "Player.h"

#include <Urho3D/DebugNew.h>

static const StringHash E_CLIENTCUSTOMEVENT("ClientCustomEvent");

// Custom remote event we use to tell the client which object they control
static const StringHash E_CLIENTOBJECTAUTHORITY("ClientObjectAuthority");
// Identifier for the node ID parameter in the event data
static const StringHash PLAYER_ID("IDENTITY");
// Custom event on server, client has pressed button that it wants to start game
static const StringHash E_CLIENTISREADY("ClientReadyToStart");

URHO3D_DEFINE_APPLICATION_MAIN(CharacterDemo)

// gameobjects
// BoidSet boids;
int numOfBoidsets = 10; // needs to be an even number for the boid splitting to work properly
int updateCycleIndex = 0;
BoidSet boids[10]; // 10 x 20 boids
Player player;
// integers for the ui texts
int timer = 100;
int timerIndex = 0;
// making lineedits for the ui text
LineEdit* scoreText;
LineEdit* timerText;
LineEdit* healthText;

float ClientTimeStep = 0.0f;

// Mouse sensitivity as degrees per pixel
const float MOUSE_SENSITIVITY = 0.1f;

// Movement speed as world units per second
const float MOVE_SPEED = 30.0f;

CharacterDemo::CharacterDemo(Context* context) : Sample(context), firstPerson_(false)
{
}

CharacterDemo::~CharacterDemo()
{
}

void CharacterDemo::Start()
{
	// Execute base class startup
	Sample::Start();
	if (touchEnabled_)
	{
		touch_ = new Touch(context_, TOUCH_SENSITIVITY);
	}

	CreateScene();
	//OpenConsoleWindow();
	// Subscribe to necessary events
	SubscribeToEvents();
	// Set the mouse mode to use in the sample
	Sample::InitMouseMode(MM_RELATIVE);
}

void CharacterDemo::CreateMainMenu()
{
	menuVisible = true;

	// Set the mouse mode to use in the sample
	InitMouseMode(MM_RELATIVE);
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	UIElement* root = ui->GetRoot();
	XMLFile* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	root->SetDefaultStyle(uiStyle); //need to set default ui style
	//Create a Cursor UI element.We want to be able to hide and show the main menu at will. When hidden, the mouse will control the camera, and when visible, the mouse will be able to interact with the GUI.
	SharedPtr<Cursor> cursor(new Cursor(context_));
	cursor->SetStyleAuto(uiStyle);
	ui->SetCursor(cursor);
	// Create the Window and add it to the UI's root node
	window_ = new Window(context_);
	root->AddChild(window_);
	// Set Window size and layout settings
	window_->SetMinWidth(384);
	window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
	window_->SetAlignment(HA_CENTER, VA_CENTER);
	window_->SetName("Window");
	window_->SetStyleAuto();

	Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

	Button* startButton = CreateButton("START", 24, window_, font);
	Button* connectButton = CreateButton("CONNECT", 24, window_, font);
	IPaddress = CreateLineEdit("", 24, window_, font);
	Button* disconnectButton = CreateButton("DISCONNECT", 24, window_, font);
	Button* startServerButton = CreateButton("START SERVER", 24, window_, font);
	Button* startClientGameButton = CreateButton("CLIENT: START GAME", 24, window_, font);
	Button* quitButton = CreateButton("QUIT", 24, window_, font);

	SubscribeToEvent(startButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleStart));
	SubscribeToEvent(quitButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleQuit));

	//Network button handlers
	SubscribeToEvent(connectButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleConnect));
	SubscribeToEvent(startServerButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleStartServer));
	SubscribeToEvent(disconnectButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleDisconnect));
	SubscribeToEvent(startClientGameButton, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleClientStartGame));
}

void CharacterDemo::CreateScoreUI()
{
	// UI Text
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* uiScore = GetSubsystem<UI>();
	UIElement* root = uiScore->GetRoot();
	XMLFile* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	root->SetDefaultStyle(uiStyle); //need to set default ui style

	// Create the Window and add it to the UI's root node
	windowScore_ = new Window(context_);
	root->AddChild(windowScore_);
	// Set Window size and layout settings
	windowScore_->SetMinWidth(120);
	windowScore_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
	windowScore_->SetAlignment(HA_LEFT, VA_TOP);
	windowScore_->SetName("Window");
	windowScore_->SetStyleAuto();

	Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

	scoreText = CreateLineEdit("Score: " + String(player.score), 24, windowScore_, font);
	timerText = CreateLineEdit("Timer: " + String(timer), 24, windowScore_, font);
	healthText = CreateLineEdit("Health:  " + String(player.health), 24, windowScore_, font);
}

void CharacterDemo::CreateScene()
{
	// -------------------- SCENE -------------------
		//so we can access resources
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	scene_ = new Scene(context_);
	// Create scene subsystem components
	scene_->CreateComponent<Octree>();
	scene_->CreateComponent<PhysicsWorld>();

	//debug shape render
	scene_->CreateComponent<DebugRenderer>();

	// -------------------- SKYBOX -------------------
	{
		//create skybox 
		Node* skyNode = scene_->CreateChild("Skybox");
		Skybox* sky = skyNode->CreateComponent<Skybox>();
		sky->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
		sky->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));
	}

	// -------------------- CAMERA -------------------
	{
		//Create camera node and component
		cameraNode_ = new Node(context_);
		Camera* camera = cameraNode_->CreateComponent<Camera>();
		cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
		camera->SetFarClip(300.0f);

		GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));
	}

	// -------------------- FOG -------------------
	{
		// Create static scene content. First create a zone for ambient lighting and fog control
		Node* zoneNode = scene_->CreateChild("Zone");
		// Zone* zone = zoneNode->CreateComponent<Zone>();
		Zone* zone = zoneNode->CreateComponent<Zone>(LOCAL);
		zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
		zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
		zone->SetFogStart(100.0f);
		zone->SetFogEnd(300.0f);
		zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
	}

	// -------------------- LIGHT -------------------
	{
		// Create a directional light with cascaded shadow mapping
		Node* lightNode = scene_->CreateChild("DirectionalLight");
		lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
		Light* light = lightNode->CreateComponent<Light>();
		// Light* light = lightNode->CreateComponent<Light>(LOCAL);
		light->SetLightType(LIGHT_DIRECTIONAL);
		light->SetCastShadows(true);
		light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
		light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
		light->SetSpecularIntensity(0.5f);
	}

	// -------------------- TERRAIN -------------------
	{
		Node* terrrainNode = scene_->CreateChild("Terrain");
		terrrainNode->SetPosition(Vector3(0.0f, -10.5f));
		Terrain* terrain = terrrainNode->CreateComponent<Terrain>();
		// Terrain* terrain = terrrainNode->CreateComponent<Terrain>(LOCAL);
		terrain->SetSpacing(Vector3(2.0f, 0.25f, 2.0f));
		terrain->SetSmoothing(true);
		terrain->SetHeightMap(cache->GetResource<Image>("Textures/HeightMap.png"));
		terrain->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
		terrain->SetCastShadows(true);
		terrain->SetOccluder(true);
	}

	player.initialise(cache, scene_, cameraNode_);
	for (int i = 0; i < numOfBoidsets; i++)
	{
		boids[i].Initialise(cache, scene_);
	}
	
	// create UI
	CreateMainMenu();
}

void CharacterDemo::SubscribeToEvents()
{
	// Subscribe to Update event for setting the character controls
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(CharacterDemo, HandleUpdate));
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(CharacterDemo, HandlePostUpdate));

	// server: what happens when a client is connected
	SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(CharacterDemo, HandleClientConnected));

	// Setting or applying controls
	SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(CharacterDemo, HandlePhysicsPreStep));

	//Server: This is called when a client has connected to a Server
	SubscribeToEvent(E_CLIENTSCENELOADED, URHO3D_HANDLER(CharacterDemo, HandleClientFinishedLoading));

	// SubscribeToEvent(E_CLIENTCUSTOMEVENT, URHO3D_HANDLER(CharacterDemo, HandleCustomEvent));
	// GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTCUSTOMEVENT);

	SubscribeToEvent(E_CLIENTISREADY, URHO3D_HANDLER(CharacterDemo, HandleClientToServerReadyToStart));
	GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTISREADY);
	SubscribeToEvent(E_CLIENTOBJECTAUTHORITY, URHO3D_HANDLER(CharacterDemo, HandleServerToClientObjectID));
	GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTOBJECTAUTHORITY);

	SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(CharacterDemo, HandlePostRender));

	// node collision
	SubscribeToEvent(player.pNode, E_NODECOLLISION, URHO3D_HANDLER(CharacterDemo, HandlePlayerCollision));
	SubscribeToEvent(player.playerMissile.pNode, E_NODECOLLISION, URHO3D_HANDLER(CharacterDemo, HandleMissileCollision));
}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	if ((GetSubsystem<Network>()->IsServerRunning() || singlePlayer) && !menuVisible)
	{
		using namespace Update;
		// Take the frame time step, which is stored as a float
		float timeStep = eventData[P_TIMESTEP].GetFloat();
		ClientTimeStep = timeStep;

		// Do not move if the UI has a focused element (the console)
		if (GetSubsystem<UI>()->GetFocusElement()) return;
		Input* input = GetSubsystem<Input>();

		// Use this frame's mouse motion to adjust camera node yaw and pitch.
		// Clamp the pitch between -90 and 90 degrees
		IntVector2 mouseMove = input->GetMouseMove();
		yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
		pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
		pitch_ = Clamp(pitch_, -90.0f, 90.0f);

		// -------------------- KEY INPUT -------------------
		{
			// Read WASD keys and move the camera scene node to the corresponding direction if they are pressed, use the Translate() functio(default local space) to move relative to the node's orientation.
			if (input->GetKeyDown(KEY_W))
			{
				//cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
				player.pNode->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
			}

			if (input->GetKeyDown(KEY_S))
			{
				//cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
				player.pNode->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
			}

			if (input->GetKeyDown(KEY_A))
			{
				//cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
				player.pNode->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
			}

			if (input->GetKeyDown(KEY_D))
			{
				//cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
				player.pNode->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
			}

			if (input->GetKeyDown(KEY_F))
			{
				if (player.playerMissile.active == false)
				{
					// shoot missile if not active already
					player.playerMissile.active = true;
					player.shoot(cameraNode_);
					// score++;
				}		
			}

			if (input->GetKeyPress(KEY_M))
			{
				menuVisible = !menuVisible;
			}
		}

		// updating half the boids at a time depending on the update cycle index
		if (updateCycleIndex == 0)
		{
			for (int i = 0; i < (numOfBoidsets/2); i++)
			{
				boids[i].Update(timeStep);
			}
			updateCycleIndex = 1;
		}
		else if (updateCycleIndex == 1)
		{
			for (int i = (numOfBoidsets / 2); i < numOfBoidsets; i++)
			{
				boids[i].Update(timeStep);
			}
			updateCycleIndex = 0;
		}

		
		player.update(cameraNode_);

		// rotating the player and making camera follow and rotate around the player
		player.pNode->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

		cameraNode_->SetPosition(player.pNode->GetPosition() + Vector3(0.0f, 2.0f, -10.0f));
		cameraNode_->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
		cameraNode_->RotateAround(player.pNode->GetPosition(), player.pNode->GetRotation(), TS_WORLD);

		// go back to menu if health reaches 0 
		if (player.health < 0)
		{
			// engine_->Exit();
			menuVisible = true;
		}

		// decriment timer every 200 update loops
		timerIndex++;
		if (timerIndex % 70 == 0 && menuVisible == false)
		{
			timer--;
		}

		// exit game if timer reacher 0
		if (timer == 0)
		{
			// engine_->Exit();
			menuVisible = !menuVisible;
		}

		// update the timer text
		timerText->SetText("Timer: " + String(timer));
	}
	else if (!menuVisible) // runs the update if it is a client and not on the menu
	{ 
		Input* input = GetSubsystem<Input>();

		// Mouse sensitivity as degrees per pixel
		const float MOUSE_SENSITIVITY = 0.1f;

		// Use this frame's mouse motion to adjust camera node yaw and pitch.
		// Clamp the pitch between -90 and 90 degrees
		IntVector2 mouseMove = input->GetMouseMove();
		yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
		pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
		pitch_ = Clamp(pitch_, -90.0f, 90.0f);
		
		// Only move the camera if we have a controllable object
		if (clientObjectID_)
		{
			Node* ClientPlayerNode = this->scene_->GetNode(clientObjectID_);
			if (ClientPlayerNode)
			{
				// making camera follow and rotate around the client player
				cameraNode_->SetPosition(ClientPlayerNode->GetPosition() + Vector3(0.0f, 2.0f, -10.0f));
				cameraNode_->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
				cameraNode_->RotateAround(ClientPlayerNode->GetPosition(), ClientPlayerNode->GetRotation(), TS_WORLD);

				// -------------------- KEY CLIENT INPUT -------------------
				{
					if (input->GetKeyPress(KEY_M))
					{
						menuVisible = !menuVisible;
					}
				}
			}
		}
	}
}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
	// menu visible & invisible
	UI* ui = GetSubsystem<UI>();
	Input* input = GetSubsystem<Input>();
	ui->GetCursor()->SetVisible(menuVisible);
	window_->SetVisible(menuVisible);
}

void CharacterDemo::HandlePostRender(StringHash eventType, VariantMap & eventData)
{
	// Drawing collision boxes around objects
	//scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
}

void CharacterDemo::HandlePlayerCollision(StringHash eventType, VariantMap & eventData)
{
	using namespace NodeCollision;

	// decrease health if player player collides with boids
	Node* collidedNode = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());
	
	if (collidedNode->GetName() == "boid")
	{
		if (!menuVisible)
		{
			// engine_->Exit();
			printf("Player collided with Boid!! \n");
			player.health = player.health - 20;
			healthText->SetText("Health: " + String(player.health));
		}
	}
}

void CharacterDemo::HandleMissileCollision(StringHash eventType, VariantMap & eventData)
{
	using namespace NodeCollision;

	// increase score if missile collides with boids
	Node* collidedNode = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());
	if (collidedNode->GetName() == "boid")
	{
		if (!menuVisible)
		{
			player.score++;
			player.playerMissile.active = false;
			scoreText->SetText("Score: " + String(player.score));

			// emitt particle effect when boid has been hit
			Node* particle = collidedNode->CreateChild("Particle");
			particle->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
			particle->SetScale(2.0f);
			ParticleEmitter* emitter = particle->CreateComponent<ParticleEmitter>();
			emitter->SetEffect(GetSubsystem<ResourceCache>()->GetResource<ParticleEffect>("Particle/Burst.xml"));
		}
	}
}

void CharacterDemo::HandleClientPlayerCollision(StringHash eventType, VariantMap & eventData)
{
	using namespace NodeCollision;

	// decrease health if player player collides with boids
	Node* collidedNode = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());

	if (collidedNode->GetName() == "boid")
	{
		if (!menuVisible)
		{
			// engine_->Exit();
			printf("Player collided with Boid!! \n");
		}
	}
}

void CharacterDemo::HandleClientMissileCollision(StringHash eventType, VariantMap & eventData)
{
	using namespace NodeCollision;
	
	// increase score if missile collides with boids
	Node* collidedNode = static_cast<Node*>(eventData[P_OTHERNODE].GetPtr());
	if (collidedNode->GetName() == "boid")
	{
		if (!menuVisible)
		{
			// emitt particle effect when boid has been hit
			Node* particle = collidedNode->CreateChild("Particle");
			particle->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
			particle->SetScale(2.0f);
			ParticleEmitter* emitter = particle->CreateComponent<ParticleEmitter>();
			emitter->SetEffect(GetSubsystem<ResourceCache>()->GetResource<ParticleEffect>("Particle/Burst.xml"));
		}
	}
}

Button * CharacterDemo::CreateButton(const String & text, int pHeight, Urho3D::Window * whichWindow, Font * font)
{
	Button* button = whichWindow->CreateChild<Button>();
	button->SetMinHeight(pHeight);
	button->SetStyleAuto();
	Text* buttonText = button->CreateChild<Text>();
	buttonText->SetFont(font, 12);
	buttonText->SetAlignment(HA_CENTER, VA_CENTER);
	buttonText->SetText(text);
	whichWindow->AddChild(button);

	return button;
}

LineEdit * CharacterDemo::CreateLineEdit(const String & text, int pHeight, Urho3D::Window * whichWindow, Font * font)
{
	LineEdit* lineEdit = whichWindow->CreateChild<LineEdit>();
	lineEdit->SetMinHeight(pHeight);
	lineEdit->SetAlignment(HA_CENTER, VA_CENTER);
	lineEdit->SetText(text);
	whichWindow->AddChild(lineEdit);
	lineEdit->SetStyleAuto();

	return lineEdit;
}

void CharacterDemo::HandleStart(StringHash eventType, VariantMap & eventData)
{
	if (menuVisible)
	{
		menuVisible = false;
	}

	CreateScoreUI();

	singlePlayer = true;

	// resetting score and timer
	timer = 100;
	player.score = 0;
	player.health = 100;
}

void CharacterDemo::HandleQuit(StringHash eventType, VariantMap & eventData)
{
	engine_->Exit();
}

void CharacterDemo::HandleConnect(StringHash eventType, VariantMap & eventData)
{
	// CreateScene();

	Network* network = GetSubsystem<Network>();
	String address = IPaddress->GetText().Trimmed();
	if (address.Empty()) { address = "localhost"; }
	//Specify scene to use as a client for replication
	network->Connect(address, SERVER_PORT, scene_);
}

void CharacterDemo::HandleStartServer(StringHash eventType, VariantMap & eventData)
{
	Log::WriteRaw("(HandleStartServer called) Server is started!");
	Network* network = GetSubsystem<Network>();
	network->StartServer(SERVER_PORT);
	// code to make your main menu disappear. Boolean value
	menuVisible = !menuVisible;
	CreateScoreUI();

	// resetting score and timer
	timer = 100;
	player.score = 0;
	player.health = 100;
}

void CharacterDemo::HandleDisconnect(StringHash eventType, VariantMap & eventData)
{
	Log::WriteRaw("HandleDisconnect has been pressed. \n");
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();
	// Running as Client
	if (serverConnection)
	{
		serverConnection->Disconnect();
		scene_->Clear(true, false);
		clientObjectID_ = 0;
	}
	// Running as a server, stop it
	else if (network->IsServerRunning())
	{
		network->StopServer();
		scene_->Clear(true, false);
	}
}

void CharacterDemo::HandleClientConnected(StringHash eventType, VariantMap & eventData)
{
	Log::WriteRaw("(HandleClientConnected) A client has connected!");
	using namespace ClientConnected;
	// When a client connects, assign to a scene
	Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
	newConnection->SetScene(scene_);
}

Controls CharacterDemo::FromClientToServerControls()
{
	Input* input = GetSubsystem<Input>();
	Controls controls;
	//Check which button has been pressed, keep track
	controls.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
	controls.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
	controls.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
	controls.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
	controls.Set(1024, input->GetKeyDown(KEY_E));
	controls.Set(CTRL_SHOOT, input->GetKeyDown(KEY_F));
	// mouse to server
	controls.yaw_ = yaw_;
	controls.pitch_ = pitch_;

	return controls;
}

void CharacterDemo::ProcessClientControls()
{
	Network* network = GetSubsystem<Network>();
	const Vector<SharedPtr<Connection> >& connections = network->GetClientConnections();
	//Server: go through every client connected
	for (unsigned i = 0; i < connections.Size(); ++i)
	{
		Connection* connection = connections[i];
		// Get the object this connection is controlling
		Player* ClientPlayer = serverObjects_[connection];
		// Client has no item connected
		if (!ClientPlayer)
		{
			continue;
		}

		// Get the last controls sent by the client
		const Controls& controls = connection->GetControls();

		// Torque is relative to the forward vector
		Quaternion rotation(0.0f, controls.yaw_, 0.0f);
		const float MOVE_TORQUE = 5.0f;

		if (controls.buttons_ & CTRL_FORWARD)
		{
			ClientPlayer->pNode->Translate(Vector3::FORWARD * MOVE_SPEED * ClientTimeStep);
		}
			
		if (controls.buttons_ & CTRL_BACK)
		{
			ClientPlayer->pNode->Translate(Vector3::BACK * MOVE_SPEED * ClientTimeStep);
		}
			
		if (controls.buttons_ & CTRL_LEFT)
		{
			ClientPlayer->pNode->Translate(Vector3::LEFT * MOVE_SPEED * ClientTimeStep);
		}
			
		if (controls.buttons_ & CTRL_RIGHT)
		{
			ClientPlayer->pNode->Translate(Vector3::RIGHT * MOVE_SPEED * ClientTimeStep);
		}

		if (controls.buttons_ & CTRL_SHOOT)
		{
			// shoot missile if not active already
			if (ClientPlayer->playerMissile.active == false)
			{
				ClientPlayer->playerMissile.active = true;
				ClientPlayer->shoot(cameraNode_);
			}
		}

		// update the client player
		ClientPlayer->update(cameraNode_);

		// change the rotation of the client player
		ClientPlayer->pNode->SetRotation(Quaternion(controls.pitch_, controls.yaw_, 0.0f));

		// node collision
		SubscribeToEvent(ClientPlayer->pNode, E_NODECOLLISION, URHO3D_HANDLER(CharacterDemo, HandleClientPlayerCollision));
		SubscribeToEvent(ClientPlayer->playerMissile.pNode, E_NODECOLLISION, URHO3D_HANDLER(CharacterDemo, HandleClientMissileCollision));
	}
}

void CharacterDemo::HandlePhysicsPreStep(StringHash eventType, VariantMap & eventData)
{
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();
	// Client: collect controls
	if (serverConnection)
	{
		serverConnection->SetPosition(cameraNode_->GetPosition()); // send camera position too
		serverConnection->SetControls(FromClientToServerControls()); // send controls to server
	}
	// Server: Read Controls, Apply them if needed
	else if (network->IsServerRunning())
	{
		ProcessClientControls(); // take data from clients, process it
	}
}

void CharacterDemo::HandleClientFinishedLoading(StringHash eventType, VariantMap & eventData)
{
	printf("Client has finished loading up the scene from the server \n");
}

void CharacterDemo::HandleCustomEvent(StringHash eventType, VariantMap & eventData)
{
	printf("This is a custom event!! \n");
}

void CharacterDemo::HandleClientStartGame(StringHash eventType, VariantMap & eventData)
{
	printf("Client has pressed START GAME \n");
	if (clientObjectID_ == 0) // Client is still observer
	{
		Network* network = GetSubsystem<Network>();
		Connection* serverConnection = network->GetServerConnection();
		if (serverConnection)
		{
			VariantMap remoteEventData;
			remoteEventData[PLAYER_ID] = 0;
			serverConnection->SendRemoteEvent(E_CLIENTISREADY, true, remoteEventData);
		}
	}
	
	menuVisible = !menuVisible;
}

void CharacterDemo::HandleServerToClientObjectID(StringHash eventType, VariantMap & eventData)
{
	clientObjectID_ = eventData[PLAYER_ID].GetUInt();
	printf("Client ID : %i \n", clientObjectID_);
}

void CharacterDemo::HandleClientToServerReadyToStart(StringHash eventType, VariantMap & eventData)
{
	printf("Event sent by the Client and running on Server: Client is ready to start the game \n");
	using namespace ClientConnected;
	Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
	// Create a controllable object for that client
	Player* newPlayer = new Player();
	newPlayer->initialise(GetSubsystem<ResourceCache>(), scene_, cameraNode_);
	
	serverObjects_[newConnection] = newPlayer;
	// Finally send the object's node ID using a remote event
	VariantMap remoteEventData;
	remoteEventData[PLAYER_ID] = newPlayer->pNode->GetID();
	newConnection->SendRemoteEvent(E_CLIENTOBJECTAUTHORITY, true, remoteEventData);
}

