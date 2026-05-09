/*
* Copyright (c) 2006-2009 Erin Catto http://www.gphysics.com
*
* Permission to use, copy, modify, distribute and sell this software
* and its documentation for any purpose is hereby granted without fee,
* provided that the above copyright notice appear in all copies.
* Erin Catto makes no representations about the suitability
* of this software for any purpose.
* It is provided "as is" without express or implied warranty.
*/

#include "box2d-lite/World.h"
#include "box2d-lite/Body.h"
#include "box2d-lite/Joint.h"

#include <algorithm>

namespace kti_b2l {

using std::vector;
using std::map;
using std::pair;

typedef map<ArbiterKey, Arbiter>::iterator ArbIter;
typedef pair<ArbiterKey, Arbiter> ArbPair;

bool World::accumulateImpulses = true;
bool World::warmStarting = true;
bool World::positionCorrection = true;

void World::Add(Body* body)
{
	bodies.push_back(body);
}

void World::Add(Joint* joint)
{
	joints.push_back(joint);
}

void World::Remove(Body* body)
{
	bodies.erase(std::remove(bodies.begin(), bodies.end(), body), bodies.end());
	for (ArbIter it = arbiters.begin(); it != arbiters.end(); )
	{
		if (it->first.body1 == body || it->first.body2 == body)
		{
			it = arbiters.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void World::Clear()
{
	bodies.clear();
	joints.clear();
	arbiters.clear();
}

void World::BroadPhase()
{
	// O(n^2) broad-phase
	for (int i = 0; i < (int)bodies.size(); ++i)
	{
		Body* bi = bodies[i];

		for (int j = i + 1; j < (int)bodies.size(); ++j)
		{
			Body* bj = bodies[j];

			if (bi->invMass == 0.0f && bj->invMass == 0.0f)
				continue;

			Arbiter newArb(bi, bj);
			ArbiterKey key(bi, bj);

			if (newArb.numContacts > 0)
			{
				ArbIter iter = arbiters.find(key);
				if (iter == arbiters.end())
				{
					arbiters.insert(ArbPair(key, newArb));
				}
				else
				{
					iter->second.Update(newArb.contacts, newArb.numContacts);
				}
			}
			else
			{
				arbiters.erase(key);
			}
		}
	}
}

void World::Step(float dt)
{
	float inv_dt = dt > 0.0f ? 1.0f / dt : 0.0f;

	// Determine overlapping bodies and update contact points.
	BroadPhase();

	// Integrate forces.
	for (int i = 0; i < (int)bodies.size(); ++i)
	{
		Body* b = bodies[i];

		if (b->invMass == 0.0f)
			continue;

		b->velocity += dt * (gravity + b->invMass * b->force);
		b->angularVelocity += dt * b->invI * b->torque;
	}

	// Perform pre-steps.
	for (ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb)
	{
		arb->second.PreStep(inv_dt);
	}

	for (int i = 0; i < (int)joints.size(); ++i)
	{
		joints[i]->PreStep(inv_dt);
	}

	// Perform iterations
	for (int i = 0; i < iterations; ++i)
	{
		for (ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb)
		{
			arb->second.ApplyImpulse();
		}

		for (int j = 0; j < (int)joints.size(); ++j)
		{
			joints[j]->ApplyImpulse();
		}
	}

	// Integrate Velocities
	for (int i = 0; i < (int)bodies.size(); ++i)
	{
		Body* b = bodies[i];

		b->position += dt * b->velocity;
		b->rotation += dt * b->angularVelocity;

		b->force.Set(0.0f, 0.0f);
		b->torque = 0.0f;
	}
}

void Joint::Set(Body* b1, Body* b2, Vec2 const& anchor)
{
	body1 = b1;
	body2 = b2;

	Mat22 Rot1(body1->rotation);
	Mat22 Rot2(body2->rotation);
	Mat22 Rot1T = Rot1.Transpose();
	Mat22 Rot2T = Rot2.Transpose();

	localAnchor1 = Rot1T * (anchor - body1->position);
	localAnchor2 = Rot2T * (anchor - body2->position);

	P.Set(0.0f, 0.0f);

	softness = 0.0f;
	biasFactor = 0.2f;
}

void Joint::PreStep(float inv_dt)
{
	Mat22 Rot1(body1->rotation);
	Mat22 Rot2(body2->rotation);

	r1 = Rot1 * localAnchor1;
	r2 = Rot2 * localAnchor2;

	Mat22 K1;
	K1.col1.x = body1->invMass + body2->invMass;
	K1.col2.x = 0.0f;
	K1.col1.y = 0.0f;
	K1.col2.y = body1->invMass + body2->invMass;

	Mat22 K2;
	K2.col1.x = body1->invI * r1.y * r1.y;
	K2.col2.x = -body1->invI * r1.x * r1.y;
	K2.col1.y = -body1->invI * r1.x * r1.y;
	K2.col2.y = body1->invI * r1.x * r1.x;

	Mat22 K3;
	K3.col1.x = body2->invI * r2.y * r2.y;
	K3.col2.x = -body2->invI * r2.x * r2.y;
	K3.col1.y = -body2->invI * r2.x * r2.y;
	K3.col2.y = body2->invI * r2.x * r2.x;

	Mat22 K = K1 + K2 + K3;
	K.col1.x += softness;
	K.col2.y += softness;

	M = K.Invert();

	Vec2 p1 = body1->position + r1;
	Vec2 p2 = body2->position + r2;
	Vec2 dp = p2 - p1;

	if (World::positionCorrection)
	{
		bias = -biasFactor * inv_dt * dp;
	}
	else
	{
		bias.Set(0.0f, 0.0f);
	}

	if (World::warmStarting)
	{
		body1->velocity -= body1->invMass * P;
		body1->angularVelocity -= body1->invI * Cross(r1, P);

		body2->velocity += body2->invMass * P;
		body2->angularVelocity += body2->invI * Cross(r2, P);
	}
	else
	{
		P.Set(0.0f, 0.0f);
	}
}

void Joint::ApplyImpulse()
{
	Vec2 dv = body2->velocity + Cross(body2->angularVelocity, r2) - body1->velocity - Cross(body1->angularVelocity, r1);

	Vec2 impulse = M * (bias - dv - softness * P);

	body1->velocity -= body1->invMass * impulse;
	body1->angularVelocity -= body1->invI * Cross(r1, impulse);

	body2->velocity += body2->invMass * impulse;
	body2->angularVelocity += body2->invI * Cross(r2, impulse);

	P += impulse;
}

} // namespace kti_b2l
