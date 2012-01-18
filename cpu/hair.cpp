#include "hair.h"
#include "constants.h"

#include <iostream>
#include <iomanip>
//#include <algorithm>
#include <cstring>
#include <cfloat>
#include <cmath>

namespace pilar
{

////////////////////////////// Particle Class //////////////////////////////////
	Particle::Particle(float mass)
	{
		this->mass = mass;
	}
	
	void Particle::clearForces()
	{
		force.x = 0.0f;
		force.y = 0.0f;
		force.z = 0.0f;
	}
	
	void Particle::applyForce(Vector3f force)
	{
		this->force += force;
	}
	
	void Particle::update(float dt)
	{
		velocity = velh * 2 - velocity;
	}
	
	void Particle::updateVelocity(float dt)
	{
		velh = velocity + force * (dt / 2.0f);
	}
	
	void Particle::updatePosition(float dt)
	{
		Vector3f newPosition = position + velh * dt;
		posh = (position + newPosition)/2.0f;
		position = newPosition;
	}

////////////////////////////// Spring Class ////////////////////////////////////

	Spring::Spring(Particle* particle1, Particle* particle2, float k, float length, float damping, SpringType type)
	{
		this->k = k;
		this->length = length;
		this->damping = damping;
		
		particle = new Particle*[2];
		
		this->particle[0] = particle1;
		this->particle[1] = particle2;
	}
	
	void Spring::update1(float dt)
	{
		//update spring forces using previous position 
		updateForce(particle[0]->position, particle[1]->position, dt);
	}
	
	void Spring::update2(float dt)
	{
		//Update spring forces using new calculated half positions
		updateForce(particle[0]->posh, particle[1]->posh, dt);
	}
	
//	void Spring::conjugate(const float* A, const float* b, float* x)
//	{
		
//	}
	
	//Calculates the current velocities and applies the spring forces
	void Spring::updateForce(Vector3f p0, Vector3f p1, float dt)
	{
		Vector3f force;
		
		Vector3f xn = p1 - p0;
		Vector3f vn = particle[1]->velocity - particle[0]->velocity;
		Vector3f d = xn * xn.length_inverse();
		
		//Calculate velocity
		float f = k / length * particle[0]->mass;
		
		Vector3f v1(particle[1]->velc.x-particle[0]->velc.x, particle[1]->velc.y-particle[0]->velc.y, particle[1]->velc.z-particle[0]->velc.z);
		
		force += d * (f * (xn.x*d.x + xn.y*d.y + xn.z*d.z - length + dt*(v1.x*d.x + v1.y*d.y + v1.z*d.z)));

//		float damp = (dt * k / (length + damping))/particle[0]->mass;
//		Vector3f friction(damp, damp, damp);		
//		force += friction;
		
		particle[0]->applyForce(force*-1.0f);
		particle[1]->applyForce(force);
//		particle[0]->applyForce(force);
//		particle[1]->applyForce(force*-1.0f);
	}
	
	void Spring::release()
	{
		particle[0] = NULL;
		particle[1] = NULL;
		
		delete [] particle;
	}
	
////////////////////////////// Strand Class ////////////////////////////////////
	
	Strand::Strand(int numParticles,
				   float mass,
				   float k_edge,
				   float k_bend,
				   float k_twist,
				   float k_extra,
				   float d_edge,
				   float d_bend,
				   float d_twist,
				   float d_extra,
				   float length,
				   Vector3f root=Vector3f())
	{
		numEdges = numParticles - 1;
		numBend  = numParticles - 2;
		numTwist = numParticles - 3;
		
		this->numParticles = numParticles;
		
		this->k_edge = k_edge;
		this->k_bend = k_bend;
		this->k_twist = k_twist;
		this->k_extra = k_extra;
		
		this->d_edge = d_edge;
		this->d_bend = d_bend;
		this->d_twist = d_twist;
		this->d_extra = d_extra;
		
		this->length = length;
		this->mass  = mass;
		
//		std::cout << mass << " " << length << " " << k_edge << std::endl;
		
		particle = new Particle*[numParticles];
		
		//Initialise particles
		for(int i = 0; i < numParticles; i++)
		{
			particle[i] = new Particle(mass);
			
			//TODO set the intial particle positions
			particle[i]->position = (root + Vector3f(length/2.0f*float(i), 0.0f, 0.0f));
			particle[i]->posc = particle[i]->position;

//			std::cout << particle[i]->position.x << " " << particle[i]->position.y << " " << particle[i]->position.z << std::endl;			
//			std::cout << particle[i]->velh.x << " " << particle[i]->velh.y << " " << particle[i]->velh.z << std::endl;
//			std::cout << particle[i]->velc.x << " " << particle[i]->velc.y << " " << particle[i]->velc.z << std::endl;
		}
		
		this->A = new float[numParticles*3*numParticles*3];
		this->x = new float[numParticles*3];
		this->b = new float[numParticles*3];
		
		buildSprings();
	}
	
	//Build the three types of spring connections between particles
	void Strand::buildSprings()
	{
		edge = new Spring*[numEdges];
		
		for(int i = 0; i < numEdges; i++)
		{
			edge[i] = new Spring(particle[i], particle[i+1], k_edge, length, d_edge, EDGE);
		}
		
		bend = new Spring*[numBend];
		
		for(int i = 0; i < numBend; i++)
		{
			bend[i] = new Spring(particle[i], particle[i+2], k_bend, length, d_bend, BEND);
		}
		
		twist = new Spring*[numTwist];
		
		for(int i = 0; i < numTwist; i++)
		{
			twist[i] = new Spring(particle[i], particle[i+3], k_twist, length, d_twist, TWIST);
		}
		
		
		//TODO add extra springs
	}
	
	void Strand::clearForces()
	{
		for(int i = 0; i < numParticles; i++)
		{
			particle[i]->clearForces();
		}
	}
	
	void Strand::updateSprings1(float dt)
	{
		for(int i = 0; i < numEdges; i++)
		{
			edge[i]->update1(dt);
		}
		
		
		for(int i = 0; i < numBend; i++)
		{
			bend[i]->update1(dt);
		}
		
		for(int i = 0; i < numTwist; i++)
		{
			twist[i]->update1(dt);
		}
		
	}
	
	void Strand::updateSprings2(float dt)
	{
		for(int i = 0; i < numEdges; i++)
		{
			edge[i]->update2(dt);
		}
		
		for(int i = 0; i < numBend; i++)
		{
			bend[i]->update2(dt);
		}
		
		for(int i = 0; i < numTwist; i++)
		{
			twist[i]->update2(dt);
		}
		
	}
	
	void Strand::updateVelocities(float dt)
	{
		for(int i = 1; i < numParticles; i++)
		{
			particle[i]->updateVelocity(dt);
		}
	}
	
	void Strand::updateParticles1(float dt)
	{
		for(int i = 1; i < numParticles; i++)
		{
			particle[i]->updatePosition(dt);
		}
	}
	
	void Strand::updateParticles2(float dt)
	{
		for(int i = 1; i < numParticles; i++)
		{
			particle[i]->updateVelocity(dt);
			particle[i]->update(dt);
		}
	}
	
	void Strand::conjugate(int N, const float* A, const float* b, float* x)
	{
		float r[N];
		float p[N];
	
		for(int i = 0; i < N; i++)
		{
			//r = b - Ax
			r[i] = b[i];
			for(int j = 0; j < N; j++)
			{
				r[i] -= A[i*N+j]*x[j];
			}
		
			//p = r
			p[i] = r[i];
		}
	
		float rsold = 0.0f;
	
		for(int i = 0; i < N; i ++)
		{
			rsold += r[i] * r[i];
		}
	
		for(int i = 0; i < N; i++)
		{
			float Ap[N];
		
			for(int j = 0; j < N; j++)
			{
				Ap[j] = 0.0f;
			
				for(int k = 0; k < N; k++)
				{
					Ap[j] += A[j*N+k] * p[k];
				}
			}
		
			float abot = 0.0f;
		
			for(int j = 0; j < N; j++)
			{
				abot += p[j] * Ap[j];
			}
		
			float alpha = rsold / abot;
		
			for(int j = 0; j < N; j++)
			{
				x[j] = x[j] + alpha * p[j];
			
				r[j] = r[j] - alpha * Ap[j];
			}
		
			float rsnew = 0.0f;
		
			for(int j = 0; j < N; j++)
			{
				rsnew += r[j] * r[j];
			}
		
			if(rsnew < 1e-10f)
			{
	//			cout << "break " << i << endl;
				break;
			}
		
			for(int j = 0; j < N; j++)
			{
				p[j] = r[j] + rsnew / rsold * p[j];
			}
		
			rsold = rsnew;
		}
	}
	
	void Strand::calcVelocities(float dt)
	{
		int N = numParticles * 3;
		
		memset(A, 0, N*N*sizeof(float));
		memset(b, 0, N*sizeof(float));
		
		for(int i = 0; i < numParticles; i++)
		{
			x[i*3]   = particle[i]->velocity.x;
			x[i*3+1] = particle[i]->velocity.y;
			x[i*3+2] = particle[i]->velocity.z;
		}
		
		//Add edge springs data into A and b
		float h = dt*dt*k_edge/(4.0f*mass*length);
		float g = dt*k_edge/(2.0f*mass*length);
	
		for(int i = 0; i < numEdges; i++)
		{
			Vector3f d(particle[i+1]->position.x-particle[i]->position.x, particle[i+1]->position.y-particle[i]->position.y, particle[i+1]->position.z - particle[i]->position.z);
			Vector3f e = d;
			d = d * d.length_inverse();
		
			A[(i*3)*N  +i*3] += 1+h*d.x*d.x; A[(i*3)*N  +i*3+1] +=   h*d.x*d.y; A[(i*3)*N  +i*3+2] +=   h*d.x*d.z; A[(i*3)*N  +i*3+3] +=  -h*d.x*d.x; A[(i*3)*N  +i*3+4] +=  -h*d.x*d.y; A[(i*3)*N  +i*3+5] +=  -h*d.x*d.z;
			A[(i*3+1)*N+i*3] +=   h*d.x*d.y; A[(i*3+1)*N+i*3+1] += 1+h*d.y*d.y; A[(i*3+1)*N+i*3+2] +=   h*d.y*d.z; A[(i*3+1)*N+i*3+3] +=  -h*d.x*d.y; A[(i*3+1)*N+i*3+4] +=  -h*d.y*d.y; A[(i*3+1)*N+i*3+5] +=  -h*d.y*d.z;
			A[(i*3+2)*N+i*3] +=   h*d.x*d.z; A[(i*3+2)*N+i*3+1] +=   h*d.y*d.z; A[(i*3+2)*N+i*3+2] += 1+h*d.z*d.z; A[(i*3+2)*N+i*3+3] +=  -h*d.x*d.z; A[(i*3+2)*N+i*3+4] +=  -h*d.y*d.z; A[(i*3+2)*N+i*3+5] +=  -h*d.z*d.z;
			A[(i*3+3)*N+i*3] +=  -h*d.x*d.x; A[(i*3+3)*N+i*3+1] +=  -h*d.x*d.y; A[(i*3+3)*N+i*3+2] +=  -h*d.x*d.z; A[(i*3+3)*N+i*3+3] += 1+h*d.x*d.x; A[(i*3+3)*N+i*3+4] +=   h*d.x*d.y; A[(i*3+3)*N+i*3+5] +=   h*d.x*d.z;
			A[(i*3+4)*N+i*3] +=  -h*d.x*d.y; A[(i*3+4)*N+i*3+1] +=  -h*d.y*d.y; A[(i*3+4)*N+i*3+2] +=  -h*d.y*d.z; A[(i*3+4)*N+i*3+3] +=   h*d.x*d.y; A[(i*3+4)*N+i*3+4] += 1+h*d.y*d.y; A[(i*3+4)*N+i*3+5] +=   h*d.y*d.z;
			A[(i*3+5)*N+i*3] +=  -h*d.x*d.z; A[(i*3+5)*N+i*3+1] +=  -h*d.y*d.z; A[(i*3+5)*N+i*3+2] +=  -h*d.z*d.z; A[(i*3+5)*N+i*3+3] +=   h*d.x*d.z; A[(i*3+5)*N+i*3+4] +=   h*d.y*d.z; A[(i*3+5)*N+i*3+5] += 1+h*d.z*d.z;
		
			float factor = g * ((e.x*d.x+e.y*d.y+e.z*d.z) - (length));
		
			b[i*3]   += particle[i]->velocity.x + factor * d.x;
			b[i*3+1] += particle[i]->velocity.y + factor * d.y;
			b[i*3+2] += particle[i]->velocity.z + factor * d.z;
			b[i*3+3] += particle[i+1]->velocity.x - factor * d.x;
			b[i*3+4] += particle[i+1]->velocity.y - factor * d.y;
			b[i*3+5] += particle[i+1]->velocity.z - factor * d.z;
		}
		
		//Add bending springs data into A and b
		h = dt*dt*k_bend/(4.0f*mass*length);
		g = dt*k_bend/(2.0f*mass*length);
		
		for(int i = 0; i < numBend; i++)
		{
			Vector3f d(particle[i+2]->position.x-particle[i]->position.x, particle[i+2]->position.y-particle[i]->position.y, particle[i+2]->position.z - particle[i]->position.z);
			Vector3f e = d;
			d = d * d.length_inverse();
			
			A[(i*3  )*N+i*3] += 1+h*d.x*d.x; A[(i*3  )*N+i*3+1] +=   h*d.x*d.y; A[(i*3  )*N+i*3+2] +=   h*d.x*d.z; A[(i*3  )*N+i*3+6] +=  -h*d.x*d.x; A[(i*3  )*N+i*3+7] +=  -h*d.x*d.y; A[(i*3  )*N+i*3+8] +=  -h*d.x*d.z;
			A[(i*3+1)*N+i*3] +=   h*d.x*d.y; A[(i*3+1)*N+i*3+1] += 1+h*d.y*d.y; A[(i*3+1)*N+i*3+2] +=   h*d.y*d.z; A[(i*3+1)*N+i*3+6] +=  -h*d.x*d.y; A[(i*3+1)*N+i*3+7] +=  -h*d.y*d.y; A[(i*3+1)*N+i*3+8] +=  -h*d.y*d.z;
			A[(i*3+2)*N+i*3] +=   h*d.x*d.z; A[(i*3+2)*N+i*3+1] +=   h*d.y*d.z; A[(i*3+2)*N+i*3+2] += 1+h*d.z*d.z; A[(i*3+2)*N+i*3+6] +=  -h*d.x*d.z; A[(i*3+2)*N+i*3+7] +=  -h*d.y*d.z; A[(i*3+2)*N+i*3+8] +=  -h*d.z*d.z;
			A[(i*3+6)*N+i*3] +=  -h*d.x*d.x; A[(i*3+6)*N+i*3+1] +=  -h*d.x*d.y; A[(i*3+6)*N+i*3+2] +=  -h*d.x*d.z; A[(i*3+6)*N+i*3+6] += 1+h*d.x*d.x; A[(i*3+6)*N+i*3+7] +=   h*d.x*d.y; A[(i*3+6)*N+i*3+8] +=   h*d.x*d.z;
			A[(i*3+7)*N+i*3] +=  -h*d.x*d.y; A[(i*3+7)*N+i*3+1] +=  -h*d.y*d.y; A[(i*3+7)*N+i*3+2] +=  -h*d.y*d.z; A[(i*3+7)*N+i*3+6] +=   h*d.x*d.y; A[(i*3+7)*N+i*3+7] += 1+h*d.y*d.y; A[(i*3+7)*N+i*3+8] +=   h*d.y*d.z;
			A[(i*3+8)*N+i*3] +=  -h*d.x*d.z; A[(i*3+8)*N+i*3+1] +=  -h*d.y*d.z; A[(i*3+8)*N+i*3+2] +=  -h*d.z*d.z; A[(i*3+8)*N+i*3+6] +=   h*d.x*d.z; A[(i*3+8)*N+i*3+7] +=   h*d.y*d.z; A[(i*3+8)*N+i*3+8] += 1+h*d.z*d.z;
			
			float factor = g * ((e.x*d.x+e.y*d.y+e.z*d.z) - (length));
			
			b[i*3  ] += particle[i]->velocity.x + factor * d.x;
			b[i*3+1] += particle[i]->velocity.y + factor * d.y;
			b[i*3+2] += particle[i]->velocity.z + factor * d.z;
			b[i*3+6] += particle[i+2]->velocity.x - factor * d.x;
			b[i*3+7] += particle[i+2]->velocity.y - factor * d.y;
			b[i*3+8] += particle[i+2]->velocity.z - factor * d.z;
		}
	
		//Add twisting springs data into A and b
		h = dt*dt*k_twist/(4.0f*mass*length);
		g = dt*k_twist/(2.0f*mass*length);
	
		for(int i = 0; i < numTwist; i++)
		{
			Vector3f d(particle[i+3]->position.x-particle[i]->position.x, particle[i+3]->position.y-particle[i]->position.y, particle[i+3]->position.z - particle[i]->position.z);
			Vector3f e = d;
			d = d * d.length_inverse();
		
			A[(i*3   )*N+i*3] += 1+h*d.x*d.x; A[(i*3   )*N+i*3+1] +=   h*d.x*d.y; A[(i*3   )*N+i*3+2] +=   h*d.x*d.z; A[(i*3   )*N+i*3+9] +=  -h*d.x*d.x; A[(i*3   )*N+i*3+10] +=  -h*d.x*d.y; A[(i*3   )*N+i*3+11] +=  -h*d.x*d.z;
			A[(i*3+1 )*N+i*3] +=   h*d.x*d.y; A[(i*3+1 )*N+i*3+1] += 1+h*d.y*d.y; A[(i*3+1 )*N+i*3+2] +=   h*d.y*d.z; A[(i*3+1 )*N+i*3+9] +=  -h*d.x*d.y; A[(i*3+1 )*N+i*3+10] +=  -h*d.y*d.y; A[(i*3+1 )*N+i*3+11] +=  -h*d.y*d.z;
			A[(i*3+2 )*N+i*3] +=   h*d.x*d.z; A[(i*3+2 )*N+i*3+1] +=   h*d.y*d.z; A[(i*3+2 )*N+i*3+2] += 1+h*d.z*d.z; A[(i*3+2 )*N+i*3+9] +=  -h*d.x*d.z; A[(i*3+2 )*N+i*3+10] +=  -h*d.y*d.z; A[(i*3+2 )*N+i*3+11] +=  -h*d.z*d.z;
			A[(i*3+9 )*N+i*3] +=  -h*d.x*d.x; A[(i*3+9 )*N+i*3+1] +=  -h*d.x*d.y; A[(i*3+9 )*N+i*3+2] +=  -h*d.x*d.z; A[(i*3+9 )*N+i*3+9] += 1+h*d.x*d.x; A[(i*3+9 )*N+i*3+10] +=   h*d.x*d.y; A[(i*3+9 )*N+i*3+11] +=   h*d.x*d.z;
			A[(i*3+10)*N+i*3] +=  -h*d.x*d.y; A[(i*3+10)*N+i*3+1] +=  -h*d.y*d.y; A[(i*3+10)*N+i*3+2] +=  -h*d.y*d.z; A[(i*3+10)*N+i*3+9] +=   h*d.x*d.y; A[(i*3+10)*N+i*3+10] += 1+h*d.y*d.y; A[(i*3+10)*N+i*3+11] +=   h*d.y*d.z;
			A[(i*3+11)*N+i*3] +=  -h*d.x*d.z; A[(i*3+11)*N+i*3+1] +=  -h*d.y*d.z; A[(i*3+11)*N+i*3+2] +=  -h*d.z*d.z; A[(i*3+11)*N+i*3+9] +=   h*d.x*d.z; A[(i*3+11)*N+i*3+10] +=   h*d.y*d.z; A[(i*3+11)*N+i*3+11] += 1+h*d.z*d.z;
			
			float factor = g * ((e.x*d.x+e.y*d.y+e.z*d.z) - (length));
			
			b[i*3   ] += particle[i]->velocity.x + factor * d.x;
			b[i*3+1 ] += particle[i]->velocity.y + factor * d.y;
			b[i*3+2 ] += particle[i]->velocity.z + factor * d.z;
			b[i*3+9 ] += particle[i+3]->velocity.x + factor * d.x;
			b[i*3+10] += particle[i+3]->velocity.y + factor * d.y;
			b[i*3+11] += particle[i+3]->velocity.z + factor * d.z;
		}
		
//		for(int i = 0; i < N; i++)
//		{
//			for(int j = 0; j < N; j++)
//			{
//				std::cout << std::setw(6) << A[i * N + j] << " "; 
//			}
//			std::cout << std::endl;
//		}
//		std::cout << std::endl;
		
		conjugate(N, A, b, x);
		
		for(int i = 0; i < numParticles; i++)
		{
			particle[i]->velc.x = x[i*3];
			particle[i]->velc.y = x[i*3+1];
			particle[i]->velc.z = x[i*3+2];
		}
	}
	
	void Strand::applyForce(Vector3f force)
	{
		//Apply external forces like gravity here
		for(int i = 0; i < numParticles; i++)
		{
			particle[i]->applyForce(force);
		}
	}
	
	void Strand::applyStrainLimiting(float dt)
	{
		bool strained = true;
		
		//TODO faster iterative strain limiting!!!
		while(strained)
		{
			strained = false;
			
			for(int i = 1; i < numParticles; i++)
			{
				//Calculate candidate position using half velocity
				particle[i]->posc = particle[i]->position + particle[i]->velh * dt;
			
				//Determine the direction of the spring between the particles
				Vector3f dir = particle[i]->posc - particle[i-1]->posc;
			
				if(dir.length_sqr() > MAX_LENGTH_SQUARED)
				{
					strained = true;
					
					//Find a valid candidate position
					particle[i]->posc = particle[i-1]->posc + (dir * (MAX_LENGTH*dir.length_inverse())); //fast length calculation
//					particle[i]->posc = particle[i-1]->posc + (dir * (MAX_LENGTH/dir.length())); //slower length calculation

					//Calculate new half velocity based on valid candidate position, i.e. add a velocity impulse
					particle[i]->velh = (particle[i]->posc - particle[i]->position)/dt;
				}
			}
		}
	}
	
	void Strand::update(float dt)
	{
		//Reset forces on particles
		clearForces();
		
		calcVelocities(dt);
		
		//Calculate and apply spring forces using previous position
		updateSprings1(dt);
		
		//Apply gravity
		applyForce(Vector3f(0.0f, GRAVITY, 0.0f));
		
		updateVelocities(dt);		
		
		applyStrainLimiting(dt);
		
		//Calculate half velocity, half position and new position
		updateParticles1(dt);
		
		//Reset forces on particles
		clearForces();
		
		calcVelocities(dt);
		
		//Calculate and apply spring forces using half position
		updateSprings2(dt);
		
		//Apply gravity
		applyForce(Vector3f(0.0f, GRAVITY, 0.0f));
		
		//Calculate half velocity and new velocity
		updateParticles2(dt);
	}
	
	//Clean up
	void Strand::release()
	{
		//Edge springs
		for(int i = 0; i < numEdges; i++)
		{
			edge[i]->release();
			delete edge[i];
			edge[i] = NULL;
		}
		delete [] edge;
		
		
		//Bending springs
		for(int i = 0; i < numBend; i++)
		{
			bend[i]->release();
			delete bend[i];
			bend[i] = NULL;
		}
		delete [] bend;
		
		//Torsion springs
		for(int i = 0; i < numTwist; i++)
		{
			twist[i]->release();
			delete twist[i];
			twist[i] = NULL;
		}
		delete [] twist;
		
		
		//Particles
		for(int i = 0; i < numParticles; i++)
		{
			delete particle[i];
			particle[i] = NULL;
		}
		delete [] particle;
		
		delete [] A;
		delete [] x;
		delete [] b;
	}
	
/////////////////////////// Hair Class /////////////////////////////////////////
	
	Hair::Hair(int numStrands,
			   int numParticles,
			   float mass,
			   float k_edge,
			   float k_bend,
			   float k_twist,
			   float k_extra,
			   float d_edge,
			   float d_bend,
			   float d_twist,
			   float d_extra,
			   float length,
			   std::vector<Vector3f> &roots)
	{
		this->numStrands = numStrands;
		
		strand = new Strand*[numStrands];
		
		for(int i = 0; i < numStrands; i++)
		{
			strand[i] = new Strand(numParticles, mass, k_edge, k_bend, k_twist, k_extra, d_edge, d_bend, d_twist, d_extra, length, roots[i]);
		}
	}
	
	Hair::Hair(int numStrands,
			   int numParticles,
			   float mass,
			   float k_edge,
			   float k_bend,
			   float k_twist,
			   float k_extra,
			   float d_edge,
			   float d_bend,
			   float d_twist,
			   float d_extra,
			   float length,
			   std::vector<Vector3f> &roots,
			   Model_OBJ &obj)
	{
		this->numStrands = numStrands;
		
		strand = new Strand*[numStrands];
		
		for(int i = 0; i < numStrands; i++)
		{
			strand[i] = new Strand(numParticles, mass, k_edge, k_bend, k_twist, k_extra, d_edge, d_bend, d_twist, d_extra, length, roots[i]);
		}
		
		initDistanceField(obj);
	}
	
	void Hair::initDistanceField(Model_OBJ &obj)
	{
		//Initialise distance field to inifinity
		for(int xx = 0; xx < DOMAIN_DIM; xx++)
			for(int yy = 0; yy < DOMAIN_DIM; yy++)
				for(int zz = 0; zz < DOMAIN_DIM; zz++)
					grid[xx][yy][zz] = FLT_MAX;
		
		//calculate triangle normal scaling factor
		float delta = 0.25f;
		float echo = CELL_WIDTH * delta;
		
		//read in each triangle with its normal data
		
		int numVertices = obj.TotalConnectedPoints / POINTS_PER_VERTEX;
		int numTriangles = obj.TotalConnectedTriangles / TOTAL_FLOATS_IN_TRIANGLE;
		
		std::cout << "Number of Vertices: " << numVertices << std::endl;
		std::cout << "Number of Triangles: " << numTriangles << std::endl;
		
		for(int i = 0; i < numTriangles; i++)
		{
			//print triangle normals
			int index = i * TOTAL_FLOATS_IN_TRIANGLE;
			
//			std::cout << obj.normals[index]     <<  " " << obj.normals[index + 1] << " " << obj.normals[index + 2] << std::endl;
//			std::cout << obj.normals[index + 3] <<  " " << obj.normals[index + 4] << " " << obj.normals[index + 5] << std::endl;
//			std::cout << obj.normals[index + 6] <<  " " << obj.normals[index + 7] << " " << obj.normals[index + 8] << std::endl;
//			std::cout << std::endl;
			
			//print triangle vertices
//			std::cout << obj.Faces_Triangles[index]   << " " << obj.Faces_Triangles[index+1] << " " << obj.Faces_Triangles[index+2] << std::endl;
//			std::cout << obj.Faces_Triangles[index+3] << " " << obj.Faces_Triangles[index+4] << " " << obj.Faces_Triangles[index+5] << std::endl;
//			std::cout << obj.Faces_Triangles[index+6] << " " << obj.Faces_Triangles[index+7] << " " << obj.Faces_Triangles[index+8] << std::endl;
//			std::cout << std::endl;
			
			//build prism
			float prism[2][TOTAL_FLOATS_IN_TRIANGLE];
			
			for(int j = 0; j < TOTAL_FLOATS_IN_TRIANGLE; j++)
			{
				prism[0][j] = obj.Faces_Triangles[index+j] + echo * obj.normals[index+j];
				prism[1][j] = obj.Faces_Triangles[index+j] - echo * obj.normals[index+j];
			}
			
			//print prism
//			std::cout << prism[0][0] << " " << prism[0][1] << " " << prism[0][2] << std::endl;
//			std::cout << prism[0][3] << " " << prism[0][4] << " " << prism[0][5] << std::endl;
//			std::cout << prism[0][6] << " " << prism[0][7] << " " << prism[0][8] << std::endl;
//			std::cout << prism[1][0] << " " << prism[1][1] << " " << prism[1][2] << std::endl;
//			std::cout << prism[1][3] << " " << prism[1][4] << " " << prism[1][5] << std::endl;
//			std::cout << prism[1][6] << " " << prism[1][7] << " " << prism[1][8] << std::endl;
//			std::cout << std::endl;
			
			float aabb[6]; //-x,-y,-z,+x,+y,+z
			aabb[0] =  FLT_MAX;
			aabb[1] =  FLT_MAX;
			aabb[2] =  FLT_MAX;
			aabb[3] = -FLT_MAX;
			aabb[4] = -FLT_MAX;
			aabb[5] = -FLT_MAX;
			
			for(int j = 0; j < (TOTAL_FLOATS_IN_TRIANGLE / POINTS_PER_VERTEX); j++)
			{
				int ii = j * POINTS_PER_VERTEX;
				
				//Minimum
				aabb[0] = std::min(prism[0][ii],   aabb[0]);
				aabb[1] = std::min(prism[0][ii+1], aabb[1]);
				aabb[2] = std::min(prism[0][ii+2], aabb[2]);
				aabb[0] = std::min(prism[1][ii],   aabb[0]);
				aabb[1] = std::min(prism[1][ii+1], aabb[1]);
				aabb[2] = std::min(prism[1][ii+2], aabb[2]);
				
				//Maximum
				aabb[3] = std::max(prism[0][ii],   aabb[3]);
				aabb[4] = std::max(prism[0][ii+1], aabb[4]);
				aabb[5] = std::max(prism[0][ii+2], aabb[5]);
				aabb[3] = std::max(prism[1][ii],   aabb[3]);
				aabb[4] = std::max(prism[1][ii+1], aabb[4]);
				aabb[5] = std::max(prism[1][ii+2], aabb[5]);
			}
			
			//print aabb
//			std::cout << "min: " << aabb[0] << " " << aabb[1] << " " << aabb[2] << std::endl;
//			std::cout << "max: " << aabb[3] << " " << aabb[4] << " " << aabb[5] << std::endl;
//			std::cout << std::endl;
			
			//normalise to the grid
			for(int j = 0; j < 2; j++)
			{
				aabb[j*3]   = (aabb[j*3]   + DOMAIN_HALF)/CELL_WIDTH;
				aabb[j*3+1] = (aabb[j*3+1] + DOMAIN_HALF+0.125f)/CELL_WIDTH;
				aabb[j*3+2] = (aabb[j*3+2] + DOMAIN_HALF)/CELL_WIDTH;
			}
			
			//print adjusted aabb
//			std::cout << "min: " << aabb[0] << " " << aabb[1] << " " << aabb[2] << std::endl;
//			std::cout << "max: " << aabb[3] << " " << aabb[4] << " " << aabb[5] << std::endl;
//			std::cout << std::endl;
			
			//round aabb
			aabb[0] = floor(aabb[0]);
			aabb[1] = floor(aabb[1]);
			aabb[2] = floor(aabb[2]);
			aabb[3] = ceil(aabb[3]);
			aabb[4] = ceil(aabb[4]);
			aabb[5] = ceil(aabb[5]);
			
			//print rounded aabb
//			std::cout << "min: " << aabb[0] << " " << aabb[1] << " " << aabb[2] << std::endl;
//			std::cout << "max: " << aabb[3] << " " << aabb[4] << " " << aabb[5] << std::endl;
//			std::cout << std::endl;
			
			int iaabb[6];
			iaabb[0] = int(aabb[0]);
			iaabb[1] = int(aabb[1]);
			iaabb[2] = int(aabb[2]);
			iaabb[3] = int(aabb[3]);
			iaabb[4] = int(aabb[4]);
			iaabb[5] = int(aabb[5]);
			
			//print integer aabb
//			std::cout << "min: " << iaabb[0] << " " << iaabb[1] << " " << iaabb[2] << std::endl;
//			std::cout << "max: " << iaabb[3] << " " << iaabb[4] << " " << iaabb[5] << std::endl;
//			std::cout << std::endl;
			
			for(int xx = iaabb[0]; xx <= iaabb[3]; xx++)
			{
				for(int yy = iaabb[1]; yy <= iaabb[4]; yy++)
				{	
					for(int zz = iaabb[2]; zz <= iaabb[5]; zz++)
					{
						float xpos = xx * CELL_WIDTH - DOMAIN_HALF + CELL_HALF;
						float ypos = yy * CELL_WIDTH - DOMAIN_HALF - 0.125f + CELL_HALF;
						float zpos = zz * CELL_WIDTH - DOMAIN_HALF + CELL_HALF;
						
						//dot product between gridpoint and triangle normal
						float dvalue = (xpos - obj.Faces_Triangles[index]) * obj.normals[index] + (ypos - obj.Faces_Triangles[index+1]) * obj.normals[index+1] + (zpos - obj.Faces_Triangles[index+2]) * obj.normals[index+2];
						
						//build edge vectors
						float edgenorm[9];
						edgenorm[0] = obj.Faces_Triangles[index+3] - obj.Faces_Triangles[index];
						edgenorm[1] = obj.Faces_Triangles[index+4] - obj.Faces_Triangles[index+1];
						edgenorm[2] = obj.Faces_Triangles[index+5] - obj.Faces_Triangles[index+2];
						edgenorm[3] = obj.Faces_Triangles[index+6] - obj.Faces_Triangles[index+3];
						edgenorm[4] = obj.Faces_Triangles[index+7] - obj.Faces_Triangles[index+4];
						edgenorm[5] = obj.Faces_Triangles[index+8] - obj.Faces_Triangles[index+5];
						edgenorm[6] = obj.Faces_Triangles[index]   - obj.Faces_Triangles[index+6];
						edgenorm[7] = obj.Faces_Triangles[index+1] - obj.Faces_Triangles[index+7];
						edgenorm[8] = obj.Faces_Triangles[index+2] - obj.Faces_Triangles[index+8];
						
						//build edge normal vectors by cross product with triangle normal
						edgenorm[0] = obj.normals[index+1] * edgenorm[2] - obj.normals[index+2] * edgenorm[1];
						edgenorm[1] = obj.normals[index+2] * edgenorm[0] - obj.normals[index]   * edgenorm[2];
						edgenorm[2] = obj.normals[index]   * edgenorm[1] - obj.normals[index+1] * edgenorm[0];
						edgenorm[3] = obj.normals[index+1] * edgenorm[5] - obj.normals[index+2] * edgenorm[4];
						edgenorm[4] = obj.normals[index+2] * edgenorm[3] - obj.normals[index]   * edgenorm[5];
						edgenorm[5] = obj.normals[index]   * edgenorm[4] - obj.normals[index+1] * edgenorm[3];
						edgenorm[6] = obj.normals[index+1] * edgenorm[8] - obj.normals[index+2] * edgenorm[7];
						edgenorm[7] = obj.normals[index+2] * edgenorm[6] - obj.normals[index]   * edgenorm[8];
						edgenorm[8] = obj.normals[index]   * edgenorm[7] - obj.normals[index+1] * edgenorm[6];
						
						//Test whether the point lies within the triangle voronoi region
						float etest[3];
						etest[0] = xpos*edgenorm[0] + ypos*edgenorm[1] + zpos*edgenorm[2] - obj.Faces_Triangles[index  ]*edgenorm[0] - obj.Faces_Triangles[index+1]*edgenorm[1] - obj.Faces_Triangles[index+2]*edgenorm[2];
						etest[1] = xpos*edgenorm[3] + ypos*edgenorm[4] + zpos*edgenorm[5] - obj.Faces_Triangles[index+3]*edgenorm[3] - obj.Faces_Triangles[index+4]*edgenorm[4] - obj.Faces_Triangles[index+5]*edgenorm[5];
						etest[2] = xpos*edgenorm[6] + ypos*edgenorm[7] + zpos*edgenorm[8] - obj.Faces_Triangles[index+6]*edgenorm[6] - obj.Faces_Triangles[index+7]*edgenorm[7] - obj.Faces_Triangles[index+8]*edgenorm[8];
						
						if(!(etest[0] < 0.0f && etest[1] < 0.0f && etest[2] < 0.0f))
						{
							//Cross products
							edgenorm[0] = obj.normals[index+1] * edgenorm[2] - obj.normals[index+2] * edgenorm[1];
							edgenorm[1] = obj.normals[index+2] * edgenorm[0] - obj.normals[index]   * edgenorm[2];
							edgenorm[2] = obj.normals[index]   * edgenorm[1] - obj.normals[index+1] * edgenorm[0];
							edgenorm[3] = obj.normals[index+1] * edgenorm[5] - obj.normals[index+2] * edgenorm[4];
							edgenorm[4] = obj.normals[index+2] * edgenorm[3] - obj.normals[index]   * edgenorm[5];
							edgenorm[5] = obj.normals[index]   * edgenorm[4] - obj.normals[index+1] * edgenorm[3];
							edgenorm[6] = obj.normals[index+1] * edgenorm[8] - obj.normals[index+2] * edgenorm[7];
							edgenorm[7] = obj.normals[index+2] * edgenorm[6] - obj.normals[index]   * edgenorm[8];
							edgenorm[8] = obj.normals[index]   * edgenorm[7] - obj.normals[index+1] * edgenorm[6];
							
							float regiontest[3][2];
							
							//Test if the point lies between the planes that define the first edge's voronoi region.
							regiontest[0][0] = -xpos*edgenorm[0] - ypos*edgenorm[1] - zpos*edgenorm[2] + obj.Faces_Triangles[index  ]*edgenorm[0] + obj.Faces_Triangles[index+1]*edgenorm[1] + obj.Faces_Triangles[index+2]*edgenorm[2];
							regiontest[0][1] =  xpos*edgenorm[0] + ypos*edgenorm[1] + zpos*edgenorm[2] - obj.Faces_Triangles[index+3]*edgenorm[0] - obj.Faces_Triangles[index+4]*edgenorm[1] - obj.Faces_Triangles[index+5]*edgenorm[2];
							//Test if the point lies between the planes that define the second edge's voronoi region.
							regiontest[1][0] = -xpos*edgenorm[3] - ypos*edgenorm[4] - zpos*edgenorm[5] + obj.Faces_Triangles[index+3]*edgenorm[3] + obj.Faces_Triangles[index+4]*edgenorm[4] + obj.Faces_Triangles[index+5]*edgenorm[5];
							regiontest[1][1] =  xpos*edgenorm[3] + ypos*edgenorm[4] + zpos*edgenorm[5] - obj.Faces_Triangles[index+6]*edgenorm[3] - obj.Faces_Triangles[index+7]*edgenorm[4] - obj.Faces_Triangles[index+8]*edgenorm[5];
							//Test if the point lies between the planes that define the third edge's voronoi region.
							regiontest[2][0] = -xpos*edgenorm[6] - ypos*edgenorm[7] - zpos*edgenorm[8] + obj.Faces_Triangles[index+6]*edgenorm[3] + obj.Faces_Triangles[index+7]*edgenorm[4] + obj.Faces_Triangles[index+8]*edgenorm[5];
							regiontest[2][1] =  xpos*edgenorm[6] + ypos*edgenorm[7] + zpos*edgenorm[8] - obj.Faces_Triangles[index  ]*edgenorm[3] - obj.Faces_Triangles[index+1]*edgenorm[4] - obj.Faces_Triangles[index+2]*edgenorm[5];
							
							if(etest[0] >= 0.0f && regiontest[0][0] < 0.0f && regiontest[0][1] < 0.0f)
							{
								
							}
							else if(etest[1] >= 0.0f && regiontest[1][0] < 0.0f && regiontest[1][1] < 0.0f)
							{
								
							}
							else if(etest[2] >= 0.0f && regiontest[2][0] < 0.0f && regiontest[2][1] < 0.0f)
							{
								
							}
							else
							{
								float dist[3];
								dist[0] = sqrtf( (xpos-obj.Faces_Triangles[index  ])*(xpos - obj.Faces_Triangles[index  ]) + (ypos-obj.Faces_Triangles[index+1])*(ypos-obj.Faces_Triangles[index+1]) + (zpos-obj.Faces_Triangles[index+2])*(zpos-obj.Faces_Triangles[index+2]));
								dist[1] = sqrtf( (xpos-obj.Faces_Triangles[index+3])*(xpos - obj.Faces_Triangles[index+3]) + (ypos-obj.Faces_Triangles[index+4])*(ypos-obj.Faces_Triangles[index+4]) + (zpos-obj.Faces_Triangles[index+5])*(zpos-obj.Faces_Triangles[index+5]));
								dist[2] = sqrtf( (xpos-obj.Faces_Triangles[index+6])*(xpos - obj.Faces_Triangles[index+6]) + (ypos-obj.Faces_Triangles[index+7])*(ypos-obj.Faces_Triangles[index+7]) + (zpos-obj.Faces_Triangles[index+8])*(zpos-obj.Faces_Triangles[index+8]));
								
								dvalue = (dvalue >= 0.0f) ? std::min(dist[0], std::min(dist[1], dist[2])) : -1 * std::min(dist[0], std::min(dist[1], dist[2]));
							}
						}
						
						grid[xx][yy][zz] = (std::abs(dvalue) < grid[xx][yy][zz]) ? dvalue : grid[xx][yy][zz];
//						std::cout << grid[xx][yy][zz] << std::endl;
					}
				}
			}
//			std::cout << std::endl;
		}
	}
	
	void Hair::update(float dt)
	{
		for(int i = 0; i < numStrands; i++)
		{
			strand[i]->update(dt);
		}
	}
	
	//Clean up
	void Hair::release()
	{
		for(int i = 0; i < numStrands; i++)
		{
			strand[i]->release();
			delete strand[i];
			strand[i] = NULL;
		}
		
		delete [] strand;
	}
}

