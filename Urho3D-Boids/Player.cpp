#include "Player.h"


// https://opengameart.org/content/low-poly-pixel-art-spaceship
// spaceship mdoel

// https://github.com/reattiva/Urho3D-Blender
// extracting to .mdl


Player::Player()
{
	pNode = nullptr;
	pRigidBody = nullptr;
	pCollisionShape = nullptr;

	offset = Vector3(0.0f, -3.0f, 10.0f);

	score = 0;
	health = 100;
}

Player::~Player()
{

}

void Player::initialise(ResourceCache * pRes, Scene * pScene, Node* camera)
{
	pNode = pScene->CreateChild("ship");
	pNode->SetPosition(Vector3(0.0f, 20.0f, 0.0f));
	pNode->SetScale(0.5f);
	
	pObject = pNode->CreateComponent<StaticModel>();
	pObject->SetModel(pRes->GetResource<Model>("Models/Circle.mdl"));
	pObject->SetMaterial(pRes->GetResource<Material>("Materials/Jack.xml"));
	pObject->SetCastShadows(true);

	pRigidBody = pNode->CreateComponent<RigidBody>();
	pRigidBody->SetMass(1.0f);
	pRigidBody->SetUseGravity(false);
	pRigidBody->SetPosition(Vector3(0.0f, 25.0f, -100.0f));
	pRigidBody->SetTrigger(true);

	pCollisionShape = pNode->CreateComponent<CollisionShape>();
	pCollisionShape->SetBox(Vector3::ONE * 4, Vector3(0.0f, 1.5f, 0.0f));

	// engine particle effects
	Node* particle = pNode->CreateChild("Particle");
	particle->SetPosition(Vector3(0.0f, 2.0f, -4.0f));
	particle->SetScale(2.0f);
	ParticleEmitter* emitter = particle->CreateComponent<ParticleEmitter>();
	emitter->SetEffect(pRes->GetResource<ParticleEffect>("Particle/Fire.xml"));

	playerMissile.Initialise(pRes, pScene, camera);
}

void Player::update(Node* camera)
{
	pRigidBody->SetLinearFactor(Vector3::ZERO);
	pRigidBody->SetAngularFactor(Vector3::ZERO);

	playerMissile.Update(camera, pNode);
}

void Player::shoot(Node* camera)
{
	playerMissile.Shoot(pNode->GetDirection(), camera, pNode);
}
