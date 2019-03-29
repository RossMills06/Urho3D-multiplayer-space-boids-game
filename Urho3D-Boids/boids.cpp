#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>
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

#include "boids.h"

float boids::Range_FAttract = 100.0f;
float boids::Range_FRepel = 20.0f;
float boids::Range_FAlign = 5.0f;
float boids::FAttract_Vmax = 5.0f;
float boids::FAttract_Factor = 4.0f;
float boids::FRepel_Factor = 4.0f;
float boids::FAlign_Factor = 2.0f;

boids::boids()
{
	pNode = nullptr;
	pRigidBody = nullptr;
	pCollisionShape = nullptr;
}

boids::~boids()
{

}

void boids::Initialise(ResourceCache *pRes, Scene *pScene)
{
	pNode = pScene->CreateChild("boid");
	pNode->SetPosition(Vector3(0.0f, 10.0f, 50.0f));
	pNode->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
	pNode->SetScale(1.0f);

	pObject = pNode->CreateComponent<StaticModel>();
	pObject->SetModel(pRes->GetResource<Model>("Models/Cone.mdl"));
	pObject->SetMaterial(pRes->GetResource<Material>("Materials/Stone.xml"));
	pObject->SetCastShadows(true);

	pRigidBody = pNode->CreateComponent<RigidBody>();
	pRigidBody->SetMass(1.0f);
	pRigidBody->SetUseGravity(false);
	pRigidBody->SetPosition(Vector3(Random(180.0f) - 90.0f, Random(40.0f), Random(180.0f) - 90.0f));
	pRigidBody->SetTrigger(true);

	pCollisionShape = pNode->CreateComponent<CollisionShape>();
	pCollisionShape->SetBox(Vector3(1.5f, 1.5f, 1.5f));

	//setting the initial velocity
	pRigidBody->SetLinearVelocity(Vector3(Random(-20, 20), 0, Random(-20, 20)));
}

void boids::ComputeForce(boids * boidList)
{
	//Attraction force

	Vector3 CoM; //centre of mass, accumulated total
	int nAttract = 0; //count number of neigbours
	//set the force member variable to zero
	force = Vector3(0, 0, 0);
	//Search Neighbourhood
	for (int i = 0; i < numberOfBoids; i++)
	{
		//the current boid?
		if (this == &boidList[i]) continue;
		//sep = vector position of this boid from current oid
		Vector3 sep = pRigidBody->GetPosition() - boidList[i].pRigidBody->GetPosition();
		float d = sep.Length(); //distance of boid
		if (d < Range_FAttract)
		{
			//with range, so is a neighbour
			CoM += boidList[i].pRigidBody->GetPosition();
			nAttract++;
		}
	}
	if (nAttract > 0)
	{
		CoM /= nAttract;
		Vector3 dir = (CoM - pRigidBody->GetPosition()).Normalized();
		Vector3 vDesired = dir * FAttract_Vmax;
		force += (vDesired - pRigidBody->GetLinearVelocity())*FAttract_Factor;
	}
	if (nAttract > 5)
	{
		// stop checking once 5 neighbours have been found
		return;
	}

	//seperation force
	Vector3 sepForce;
	int nRepel = 0;
	for (int i = 0; i < numberOfBoids; i++)
	{
		//the current boid?
		if (this == &boidList[i]) continue;
		//sep = vector position of this boid from current oid
		Vector3 sep = pRigidBody->GetPosition() - boidList[i].pRigidBody->GetPosition();
		float d = sep.Length(); //distance of boid
		if (d < Range_FRepel)
		{
			sepForce += (sep / sep.Length());
			nRepel++;
		}
	}
	if (nRepel > 0)
	{
		sepForce *= FRepel_Factor;
		force += sepForce;
	}
	if (nRepel > 5)
	{
		// stop checking once 5 neighbours have been found
		return;
	}

	//Allignment direction
	Vector3 align;
	int nAlign = 0;
	for (int i = 0; i < numberOfBoids; i++)
	{
		//the current boid?
		if (this == &boidList[i]) continue;
		//sep = vector position of this boid from current oid
		Vector3 sep = pRigidBody->GetPosition() - boidList[i].pRigidBody->GetPosition();
		float d = sep.Length(); //distance of boid
		if (d < Range_FAlign)
		{
			align += boidList[i].pRigidBody->GetLinearVelocity();
			nAlign++;
		}
	}
	if (nAlign > 0)
	{
		align /= nAlign;

		Vector3 finalVel = align;

		force += (finalVel - pRigidBody->GetLinearVelocity()) * FAlign_Factor;
	}
	if (nAlign > 5)
	{
		// stop checking once 5 neighbours have been found
		return;
	}
}

void boids::Update(float lastFrame)
{
	pRigidBody->ApplyForce(force);
	Vector3 vel = pRigidBody->GetLinearVelocity();
	
	float d = vel.Length();
	if (d < 10.0f)
	{
		d = 10.0f;
		pRigidBody->SetLinearVelocity(vel.Normalized()*d);
	}
	else if (d > 50.0f)
	{
		d = 50.0f;
		pRigidBody->SetLinearVelocity(vel.Normalized()*d);
	}

	Quaternion endRot = Quaternion(0, 0, 0);
	Vector3 nVel = vel.Normalized();
	endRot.FromLookRotation(nVel, Vector3::UP);
	endRot = endRot * Quaternion(90, 0, 0);
	pRigidBody->SetRotation(endRot);
	
	Vector3 p = pRigidBody->GetPosition();
	if (p.y_ < 10.0f)
	{
		p.y_ = 10.0f;
		pRigidBody->SetPosition(p);
	}
	else if (p.y_ > 150.0f)
	{
		p.y_ = 150.0f;
		pRigidBody->SetPosition(p);
	}
}

BoidSet::BoidSet()
{

}

void BoidSet::Initialise(ResourceCache *pRes, Scene *pScene)
{
	for (int i = 0; i < numberOfBoids; i++)
	{
		boidList[i].Initialise(pRes, pScene);
	}
}

void BoidSet::Update(float tm)
{
	for (int i = 0; i < numberOfBoids; i++)
	{
		boidList[i].ComputeForce(&boidList[0]);
		boidList[i].Update(tm);
	}
}


