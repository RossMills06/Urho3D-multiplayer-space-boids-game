#include "Missile.h"

Missile::Missile()
{
	pNode = nullptr;
	pRigidBody = nullptr;
	pCollisionShape = nullptr;

	timer = 0.0f;
	active = false;
}

Missile::~Missile()
{
}

void Missile::Initialise(ResourceCache * pRes, Scene * pScene, Node* camera)
{
	pNode = pScene->CreateChild("missile");
	pNode->SetPosition(Vector3(0.0f, 10.0f, 50.0f));
	pNode->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
	pNode->SetScale(1.5f);

	pObject = pNode->CreateComponent<StaticModel>();
	pObject->SetModel(pRes->GetResource<Model>("Models/Torus.mdl"));
	pObject->SetMaterial(pRes->GetResource<Material>("Materials/Stone.xml"));
	pObject->SetCastShadows(true);

	pRigidBody = pNode->CreateComponent<RigidBody>();
	pRigidBody->SetMass(1.0f);
	pRigidBody->SetUseGravity(false);
	pRigidBody->SetPosition(camera->GetPosition());
	pRigidBody->SetTrigger(true);

	pCollisionShape = pNode->CreateComponent<CollisionShape>();
	pCollisionShape->SetBox(Vector3::ONE);

	pObject->SetEnabled(false);
}

void Missile::Update(Node* camera, Node* player)
{
	if (active)
	{
		pObject->SetEnabled(true);
		timer++;	
	}
	else
	{
		pRigidBody->SetPosition(player->GetPosition() + Vector3(player->GetDirection() * 3));
		pRigidBody->SetLinearVelocity(Vector3::ZERO);
		pObject->SetEnabled(false);
	}

	if (timer == 100)
	{
		active = false;
		pRigidBody->SetPosition(player->GetPosition() + Vector3(player->GetDirection() * 3));
		pRigidBody->SetLinearVelocity(Vector3::ZERO);
		pObject->SetEnabled(false);
		timer = 0; 
	}
}

void Missile::Shoot(Vector3 direction, Node * camera, Node* player)
{
	if (active)
	{
		pRigidBody->SetPosition(player->GetPosition() + Vector3(0.0f, 1.0f, 1.0f));
		pRigidBody->SetLinearVelocity(direction * 500.0f);
		timer++;
	}
	else
	{
		pRigidBody->SetPosition(camera->GetPosition());
		pRigidBody->SetLinearVelocity(Vector3::ZERO);
	}
}


