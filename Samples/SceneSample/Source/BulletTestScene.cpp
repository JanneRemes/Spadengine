#include "BulletTestScene.h"

#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Renderer/Enumerations.h"
#include "Renderer/Window.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Pipeline.h"
#include "Renderer/Shader.h"
#include "Renderer/VertexLayout.h"

void BulletTestScene::loadTextShader(const std::string& path, std::vector<char>& data)
{
	std::ifstream file;

	file.open(path, std::ios::in);

	if (file.is_open())
	{
		std::cout << "Text shader opened!" << std::endl;

		std::stringstream stream;
		std::string str;

		stream << file.rdbuf();

		str = stream.str();

		std::copy(str.begin(), str.end(), std::back_inserter(data));

		data.push_back('\0');
	}
}

void BulletTestScene::loadBinaryShader(const std::string& path, std::vector<char>& data)
{
	std::ifstream file;

	file.open(path, std::ios::in | std::ios::ate | std::ios::binary);

	if (file.is_open())
	{
		std::cout << "Binary shader opened!" << std::endl;
		data.resize(static_cast<size_t>(file.tellg()));

		file.seekg(0, std::ios::beg);
		file.read(data.data(), data.size());
		file.close();
	}
}

BulletTestScene::BulletTestScene(sge::Spade* engine) : engine(engine)
{
	std::vector<char> pShaderData;
	std::vector<char> vShaderData;

#ifdef DIRECTX11
	loadBinaryShader("../Assets/Shaders/VertexShader.cso", vShaderData);
	loadBinaryShader("../Assets/Shaders/PixelShader.cso", pShaderData);
#elif OPENGL4
	loadTextShader("../Assets/Shaders/VertexShader.glsl", vShaderData);
	loadTextShader("../Assets/Shaders/PixelShader.glsl", pShaderData);
#endif

	cameraPos = glm::vec3(5.0f, 10.0f, 50.0f);
	cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	P = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.f);
	V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	uniformData2.M = glm::translate(glm::mat4(), glm::vec3(0.0f, 50.0f, 0.0f));
	uniformData2.PV = P * V;

	//Assimp test
	modelHandle = engine->getResourceManager()->load<sge::ModelResource>("../Assets/cube.dae");
	modelHandle.getResource<sge::ModelResource>()->setRenderer(engine->getRenderer());

	sge::VertexLayoutDescription vertexLayoutDescription = { 5,
	{
		{ 0, 3, sge::VertexSemantic::POSITION },
		{ 0, 3, sge::VertexSemantic::NORMAL },
		{ 0, 3, sge::VertexSemantic::TANGENT },
		{ 0, 3, sge::VertexSemantic::TANGENT },
		{ 0, 2, sge::VertexSemantic::TEXCOORD }
	} };

	vertexShader = engine->getRenderer()->getDevice().createShader(sge::ShaderType::VERTEX, vShaderData.data(), vShaderData.size());
	pixelShader = engine->getRenderer()->getDevice().createShader(sge::ShaderType::PIXEL, pShaderData.data(), pShaderData.size());

	vertices = modelHandle.getResource<sge::ModelResource>()->getVerticeArray();
	indices = modelHandle.getResource<sge::ModelResource>()->getIndexArray();

	texture = modelHandle.getResource<sge::ModelResource>()->getDiffuseTexture();
	texture2 = modelHandle.getResource<sge::ModelResource>()->getNormalTexture();

	pipeline = engine->getRenderer()->getDevice().createPipeline(&vertexLayoutDescription, vertexShader, pixelShader);
	viewport = { 0, 0, 1280, 720 };

	vertexBuffer = engine->getRenderer()->getDevice().createBuffer(sge::BufferType::VERTEX, sge::BufferUsage::DYNAMIC, sizeof(Vertex) * vertices->size());
	uniformBuffer = engine->getRenderer()->getDevice().createBuffer(sge::BufferType::UNIFORM, sge::BufferUsage::DYNAMIC, sizeof(UniformData2));

	engine->getRenderer()->getDevice().bindViewport(&viewport);
	engine->getRenderer()->getDevice().bindPipeline(pipeline);

	engine->getRenderer()->getDevice().bindVertexBuffer(vertexBuffer);
	engine->getRenderer()->getDevice().bindVertexUniformBuffer(uniformBuffer, 0);
	engine->getRenderer()->getDevice().bindTexture(texture, 0);
	engine->getRenderer()->getDevice().bindTexture(texture2, 1);

	engine->getRenderer()->getDevice().copyData(vertexBuffer, sizeof(Vertex) * vertices->size(), vertices->data());
	engine->getRenderer()->getDevice().copyData(uniformBuffer, sizeof(uniformData2), &uniformData2);

	// Bullet test
	broadphase = new btDbvtBroadphase();

	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);

	solver = new btSequentialImpulseConstraintSolver;

	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

	dynamicsWorld->setGravity(btVector3(0, -10, 0));


	groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(1.), btScalar(50.)));

	fallShape = new btBoxShape(btVector3(1,1,1));


	btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)));
	btRigidBody::btRigidBodyConstructionInfo
		groundRigidBodyCI(0, groundMotionState, groundShape, btVector4(0, 0, 1,1));
	groundRigidBody = new btRigidBody(groundRigidBodyCI);
	dynamicsWorld->addRigidBody(groundRigidBody);


	btDefaultMotionState* fallMotionState =
		new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 50, 0)));
	btScalar mass = 1;
	btVector3 fallInertia(0, 0, 0);
	fallShape->calculateLocalInertia(mass, fallInertia);
	btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, fallMotionState, fallShape, fallInertia);
	fallRigidBody = new btRigidBody(fallRigidBodyCI);
	dynamicsWorld->addRigidBody(fallRigidBody);
}

BulletTestScene::~BulletTestScene()
{
	dynamicsWorld->removeRigidBody(fallRigidBody);
	delete fallRigidBody->getMotionState();
	delete fallRigidBody;

	dynamicsWorld->removeRigidBody(groundRigidBody);
	delete groundRigidBody->getMotionState();
	delete groundRigidBody;


	delete fallShape;

	delete groundShape;


	delete dynamicsWorld;
	delete solver;
	delete collisionConfiguration;
	delete dispatcher;
	delete broadphase;

	engine->getRenderer()->getDevice().debindPipeline(pipeline);

	engine->getRenderer()->getDevice().deleteBuffer(vertexBuffer);
	engine->getRenderer()->getDevice().deleteBuffer(uniformBuffer);

	engine->getRenderer()->getDevice().deleteShader(vertexShader);
	engine->getRenderer()->getDevice().deleteShader(pixelShader);
	engine->getRenderer()->getDevice().deleteTexture(texture);
	engine->getRenderer()->getDevice().deleteTexture(texture2);

	engine->getRenderer()->getDevice().deletePipeline(pipeline);

	engine->getResourceManager()->release(modelHandle);
}

void BulletTestScene::update(float step)
{
	dynamicsWorld->stepSimulation(step, 10);

	btTransform trans;
	fallRigidBody->getMotionState()->getWorldTransform(trans);

	std::cout << "Box height: " << trans.getOrigin().getY() << std::endl;

	// t�m� rotate tehd��n n�in! Muuten tulee salmiakkia.
	uniformData2.M = sge::math::translate(sge::math::mat4(), sge::math::vec3(trans.getOrigin().getX(), trans.getOrigin().getY(), trans.getOrigin().getZ()));

	if (engine->mouseInput->buttonIsPressed(sge::MOUSE_BUTTON_LEFT))
	{
		// Proper way to shutdown the program?
		engine->stop();
	}
}
void BulletTestScene::draw()
{
	engine->getRenderer()->getDevice().clear(0.5f, 0.0f, 0.5f, 1.0f);

	engine->getRenderer()->getDevice().copyData(uniformBuffer, sizeof(uniformData2), &uniformData2);

	engine->getRenderer()->getDevice().draw(vertices->size());

	engine->getRenderer()->getDevice().swap();
}

void BulletTestScene::interpolate(float alpha)
{
}
