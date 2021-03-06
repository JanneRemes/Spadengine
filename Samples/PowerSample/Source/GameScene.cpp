#include "GameScene.h"

#include "Game/Entity.h"
#include "Game/CameraComponent.h"
#include "Game/ModelComponent.h"
#include "Game/SpriteComponent.h"
#include "Game/TransformComponent.h"
#include "Game/RenderSystem.h"

#include "Renderer/Pipeline.h"
#include "Renderer/RenderTarget.h"
#include "Renderer/VertexLayout.h"

#include "Resources/ShaderResource.h"

#include "Spade/Spade.h"

/*
TODO enko

Tekstuuriresurssille tekstuurien generointi GPU:n muistiin
SpotLightComponent do!
Instanced rendering do!
Deferred rendering do!
Kameralle lookAt!
Optimoi bulletscenea (nykii ihan vitusti)
    - Esim valojen dataa ei tarvii vied� joka objektille erikseen
    - Sorttaus takasin kuntoon jos tarviijaksaahaluaa
Tee se demo ja siihen kaikkea hauskaa (blur, hdr voe pojat efektej�)
DX11 fixes go!
*/

GameScene::GameScene(sge::Spade* engine) :
    engine(engine),
    renderer(engine->getRenderer()),
    device(renderer->getDevice())
{
    initPipelines();
    initResources();

    overviewCamera = createPerspectiveCamera(0, 0, 620, 700);
	spaceShipCamera = createPerspectiveCamera(0, 0, 620, 340);
	earthCamera = createPerspectiveCamera(0, 0, 620, 340);

    fullscreenCamera = createOrthoCamera(0, 0, 1280, 720);

    earth = createEarth();
    sun = createSun();
    skybox = createSkyBox();

    fullScreen = createSprite(0, 0, 1280, 720, fullScreenTarget->textures[0]);
	overviewScreen = createSprite(0, 0, 620, 710, overviewScreenTarget->textures[0]);
	earthScreen = createSprite(650, 0, 620, 340, earthScreenTarget->textures[0]);
	spaceShipScreen = createSprite(650, 340, 620, 340, spaceShipScreenTarget->textures[0]);

	spaceShip = createSpaceShip();
	moon = createMoon();
}

GameScene::~GameScene()
{
    sge::ResourceManager::getMgr().release(earthResource);
    sge::ResourceManager::getMgr().release(sunResource);
    sge::ResourceManager::getMgr().release(skyBoxResource);
	sge::ResourceManager::getMgr().release(spaceShipResource);
	sge::ResourceManager::getMgr().release(moonResource);

    device->deleteCubeMap(skyBoxCubeMap);

    device->deleteShader(vertexShader);
    device->deleteShader(pixelShader);
    device->deleteShader(skyBoxPixelShader);
    device->deleteShader(skyBoxVertexShader);

    device->deletePipeline(pipeline);
    device->deletePipeline(skyBoxPipeline);

	device->deleteRenderTarget(fullScreenTarget);
}

void GameScene::update(float step)
{
    static float alpha = 0.0f;

    alpha += 0.025f;

    if (engine->keyboardInput->keyIsPressed(sge::KEYBOARD_ESCAPE))
    {
        engine->stop();
    }

    earth->getComponent<sge::TransformComponent>()->setPosition(sge::math::vec3(5.0f*cos(alpha), 0.0f, 5.0f*sin(alpha)));
    earth->getComponent<sge::TransformComponent>()->addAngle(0.025f);

	spaceShip->getComponent<sge::TransformComponent>()->setPosition(sge::math::vec3(7.5f*cos(-alpha), 1.5f * sin(alpha), 7.5f*sin(-alpha)));
	//spaceShip->getComponent<sge::TransformComponent>()->addAngle(0.015f);

    sun->getComponent<sge::TransformComponent>()->addAngle(0.01f);

	overviewCamera->getComponent<sge::TransformComponent>()->lookAt(sun->getComponent<sge::TransformComponent>()->getPosition());
	overviewCamera->getComponent<sge::CameraComponent>()->update();

    fullscreenCamera->getComponent<sge::CameraComponent>()->update();

	spaceShipCamera->getComponent<sge::TransformComponent>()->lookAt(spaceShip->getComponent<sge::TransformComponent>()->getPosition());
	spaceShipCamera->getComponent<sge::TransformComponent>()->setPosition(spaceShip->getComponent<sge::TransformComponent>()->getPosition() - spaceShip->getComponent<sge::TransformComponent>()->getFront() * 0.5f);
	spaceShipCamera->getComponent<sge::CameraComponent>()->update();

	earthCamera->getComponent<sge::TransformComponent>()->lookAt(earth->getComponent<sge::TransformComponent>()->getPosition());
	earthCamera->getComponent<sge::TransformComponent>()->setPosition(earth->getComponent<sge::TransformComponent>()->getPosition() - earth->getComponent<sge::TransformComponent>()->getFront() * 2.0f);
	earthCamera->getComponent<sge::CameraComponent>()->update();

	moon->getComponent<sge::TransformComponent>()->setPosition(earth->getComponent<sge::TransformComponent>()->getPosition() + sge::math::vec3(cos(alpha * 3.0f), 0.0f, sin(alpha * 3.0f)));
	moon->getComponent<sge::TransformComponent>()->addAngle(0.05f);
}

void GameScene::draw()
{
	// Render overview camera.
	renderer->addCameras(1, &overviewCamera);
	renderer->setRenderTarget(overviewScreenTarget);

    renderer->clear(sge::COLOR);

    renderer->begin();
    renderer->renderLights(1, &sun);
    renderer->renderModels(1, &earth);
    renderer->renderModels(1, &sun);
    renderer->renderModels(1, &skybox);
	renderer->renderModels(1, &spaceShip);
	renderer->renderModels(1, &moon);
    renderer->end();

    renderer->render();

	// Render earth camera.
	renderer->clear(sge::CAMERAS | sge::RENDERTARGET);
	renderer->addCameras(1, &earthCamera);
	renderer->setRenderTarget(earthScreenTarget);
	renderer->clear(sge::COLOR);

	renderer->render();

	// Render spaceship camera.
	renderer->clear(sge::CAMERAS | sge::RENDERTARGET);
	renderer->addCameras(1, &spaceShipCamera);
	renderer->setRenderTarget(spaceShipScreenTarget);
	renderer->clear(sge::COLOR);

	renderer->render();

	// Render cameras.
	renderer->clear();
	renderer->addCameras(1, &fullscreenCamera);
	//renderer->setRenderTarget(fullScreenTarget);

	renderer->begin();
	renderer->renderSprites(1, &overviewScreen);
	renderer->renderSprites(1, &earthScreen);
	renderer->renderSprites(1, &spaceShipScreen);
	renderer->end();

	//renderer->render();

	//// Render fullscreen.
	//renderer->clear();
 //   renderer->addCameras(1, &fullscreenCamera);

 //   renderer->begin();
 //   renderer->renderSprites(1, &fullScreen);
 //   renderer->end();

    renderer->render();
    renderer->present();
    renderer->clear();
}

sge::Entity* GameScene::createEarth()
{
    device->bindPipeline(pipeline);

    sge::Entity* entity = entityManager.createEntity();

    auto transform = transformFactory.create(entity);
    auto model = modelFactory.create(entity);

    transform->setPosition({ 0.0f, 0.0f, 5.0f });
    transform->setScale({ 0.5f, 0.5f, 0.5f });

    model->setPipeline(pipeline);
    model->setShininess(2.0f);
    model->setRenderer(renderer);
    model->setModelResource(&earthResource);

    device->debindPipeline(pipeline);

    return entity;
}

sge::Entity* GameScene::createPerspectiveCamera(int x, int y, unsigned int width, unsigned int height)
{
    sge::Entity* entity = entityManager.createEntity();

    auto transform = transformFactory.create(entity);
    auto camera = cameraFactory.create(entity);

    transform->setPosition({ 0.0f, 5.0f, -15.0f });
    transform->setUp({ 0.0f, 1.0f, 0.0f });
    transform->setRotationVector({ 0.0f, 0.0f, 1.0f });

    camera->setPerspective(60.0f, (float)width / height, 0.1f, 1000.0f);
    camera->setViewport(x, y, width, height);

    return entity;
}

sge::Entity* GameScene::createOrthoCamera(int x, int y, unsigned int width, unsigned int height)
{
    sge::Entity* entity = entityManager.createEntity();

    auto transform = transformFactory.create(entity);
    auto cameracomponent = cameraFactory.create(entity);

    transform->setPosition({ 0.0f, 0.0f, 10.0f });
    transform->setFront({ 0.0f, 0.0f, -1.0f });
    transform->setUp({ 0.0f, 1.0f, 0.0f });

    cameracomponent->setOrtho(0.0f, (float)width, 0.0f, (float)height, 0.1f, 1000.0f);
    cameracomponent->setViewport(x, y, width, height);

    return entity;
}

sge::Entity* GameScene::createSun()
{
    sge::Entity* entity = entityManager.createEntity();

    device->bindPipeline(noLightsPipeline);

    auto transform = transformFactory.create(entity);
    auto light = pointLightFactory.create(entity);
	auto dirlight = dirLightFactory.create(entity);
    auto model = modelFactory.create(entity);

    transform->setPosition({ 0.0f, 0.0f, 0.0f });
    transform->setFront({ 0.0f, 0.0f, 0.0f });
    transform->setUp({ 0.0f, 1.0f, 0.0f });
    transform->setRotationVector({ 0.0f, 1.0f, 0.0f });
    transform->setScale({ 2.0f, 2.0f, 2.0f });

    sge::PointLight lightData;

    lightData.position = sge::math::vec4(0.0f);
    lightData.constant = 1.0f;
    lightData.mylinear = 0.022f;
    lightData.quadratic = 0.0019f;
    lightData.pad = 0.0f;
    lightData.ambient = sge::math::vec4(0.0125f, 0.0125f, 0.05f, 1.0f);
    lightData.diffuse = sge::math::vec4(0.8f, 0.8f, 0.0f, 1.0f);
    lightData.specular = sge::math::vec4(0.25f, 0.25f, 1.0f, 1.0f);

    light->setLightData(lightData);

	sge::DirLight dirlightData;

	dirlightData.direction = sge::math::vec4(0.0f, 0.0f, 1.0f, 1.0);
	dirlightData.ambient = sge::math::vec4(0.05, 0.05, 0.05, 1.0);
	dirlightData.diffuse = sge::math::vec4(0.1, 0.1, 0.1, 1.0);
	dirlightData.specular = sge::math::vec4(0.2, 0.2, 0.2, 1.0);

	dirlight->setLightData(dirlightData);

    model->setPipeline(noLightsPipeline);
    model->setShininess(256.0f);
    model->setRenderer(renderer);
    model->setModelResource(&sunResource);

    device->debindPipeline(noLightsPipeline);

    return entity;
}

sge::Entity* GameScene::createSkyBox()
{
    sge::Handle<sge::TextureResource> texture;

    texture = sge::ResourceManager::getMgr().load<sge::TextureResource>("../Assets/CubeMap/space.png");

    sge::TextureResource* source[6];

    for (size_t i = 0; i < 6; i++)
    {
        source[i] = texture.getResource<sge::TextureResource>();
    }

    device->bindPipeline(skyBoxPipeline);

    sge::Entity* entity = entityManager.createEntity();

    skyBoxCubeMap = device->createCubeMap(source);

    auto transform = transformFactory.create(entity);
    auto model = modelFactory.create(entity);

    transform->setPosition({ 0.0f, 0.0f, 0.0f });
    transform->setFront({ 0.0f, 0.0f, 0.0f });
    transform->setUp({ 0.0f, 0.0f, 0.0f });

    model->setPipeline(skyBoxPipeline);
    model->setRenderer(renderer);
    model->setModelResource(&skyBoxResource);
    model->setCubeMap(skyBoxCubeMap);

    device->debindPipeline(skyBoxPipeline);

    return entity;
}

sge::Entity* GameScene::createSprite(int x, int y, unsigned int width, unsigned int height, sge::Texture* texture)
{
    sge::Entity* entity = entityManager.createEntity();

    auto transform = transformFactory.create(entity);
    auto sprite = spriteFactory.create(entity);

    transform->setPosition({ x + width / 2, y + height / 2, 1.0f });
    transform->setScale({ width / 2, height / 2, 1.0f });
    transform->setRotationVector({ 0.0f, 0.0f, 1.0f });

    sprite->setTexture(texture);
    sprite->setColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    return entity;
}

sge::Entity* GameScene::createSpaceShip()
{
	device->bindPipeline(pipeline);

	sge::Entity* entity = entityManager.createEntity();

	auto transform = transformFactory.create(entity);
	auto model = modelFactory.create(entity);

	transform->setPosition({ 3.0f, 0.0f, -5.0f });
	transform->setScale({ 0.0125f, 0.0125f, 0.0125f });
	transform->setRotationVector({ 1.0f, 0.8f, 0.5f });

	model->setPipeline(pipeline);
	model->setShininess(128.0f);
	model->setRenderer(renderer);
	model->setModelResource(&spaceShipResource);

	device->debindPipeline(pipeline);

	return entity;
}

sge::Entity* GameScene::createMoon()
{
	device->bindPipeline(pipeline);

	sge::Entity* entity = entityManager.createEntity();

	auto transform = transformFactory.create(entity);
	auto model = modelFactory.create(entity);

	transform->setPosition({ 0.0f, 0.0f, 6.0f });
	transform->setScale({ 0.1f, 0.1f, 0.1f });

	model->setPipeline(pipeline);
	model->setShininess(2.0f);
	model->setRenderer(renderer);
	model->setModelResource(&moonResource);

	device->debindPipeline(pipeline);

	return entity;
}

void GameScene::initPipelines()
{
    sge::Handle<sge::ShaderResource> pixelShaderHandle;
    sge::Handle<sge::ShaderResource> vertexShaderHandle;

    sge::Handle<sge::ShaderResource> skyBoxPixelShaderHandle;
    sge::Handle<sge::ShaderResource> skyBoxVertexShaderHandle;

    sge::Handle<sge::ShaderResource> noLightsVertexShaderHandle;
    sge::Handle<sge::ShaderResource> noLightsPixelShaderHandle;

#ifdef DIRECTX11
    vertexShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/VertexShaderLights.cso");
    pixelShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/PixelShaderLights.cso");
    skyBoxVertexShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/VertexSkyBox.cso");
    skyBoxPixelShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/PixelSkyBox.cso");
    noLightsVertexShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/VertexShaderNoLights.cso");
    noLightsPixelShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/PixelShaderNoLights.cso");
#elif OPENGL4
    vertexShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/VertexShaderLights.glsl");
    pixelShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/PixelShaderLights.glsl");
    skyBoxVertexShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/VertexSkyBox.glsl");
    skyBoxPixelShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/PixelSkyBox.glsl");
    noLightsVertexShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/VertexShaderNoLights.glsl");
    noLightsPixelShaderHandle = sge::ResourceManager::getMgr().load<sge::ShaderResource>("../Assets/Shaders/PixelShaderNoLights.glsl");
#endif

    const std::vector<char>& vertexShaderData = vertexShaderHandle.getResource<sge::ShaderResource>()->loadShader();
    const std::vector<char>& pixelShaderData = pixelShaderHandle.getResource<sge::ShaderResource>()->loadShader();

    const std::vector<char>& vertexShaderData2 = skyBoxVertexShaderHandle.getResource<sge::ShaderResource>()->loadShader();
    const std::vector<char>& pixelShaderData2 = skyBoxPixelShaderHandle.getResource<sge::ShaderResource>()->loadShader();

    const std::vector<char>& vertexShaderData3 = noLightsVertexShaderHandle.getResource<sge::ShaderResource>()->loadShader();
    const std::vector<char>& pixelShaderData3 = noLightsPixelShaderHandle.getResource<sge::ShaderResource>()->loadShader();

    vertexShader = device->createShader(sge::ShaderType::VERTEX, vertexShaderData.data(), vertexShaderData.size());
    pixelShader = device->createShader(sge::ShaderType::PIXEL, pixelShaderData.data(), pixelShaderData.size());

    skyBoxVertexShader = device->createShader(sge::ShaderType::VERTEX, vertexShaderData2.data(), vertexShaderData2.size());
    skyBoxPixelShader = device->createShader(sge::ShaderType::PIXEL, pixelShaderData2.data(), pixelShaderData2.size());

    noLightsVertexShader = device->createShader(sge::ShaderType::VERTEX, vertexShaderData3.data(), vertexShaderData3.size());
    noLightsPixelShader = device->createShader(sge::ShaderType::PIXEL, pixelShaderData3.data(), pixelShaderData3.size());

    sge::VertexLayoutDescription vertexLayoutDescription = { 5,
    {
        { 0, 3, sge::VertexSemantic::POSITION },
        { 0, 3, sge::VertexSemantic::NORMAL },
        { 0, 3, sge::VertexSemantic::TANGENT },
        { 0, 3, sge::VertexSemantic::TANGENT },
        { 0, 2, sge::VertexSemantic::TEXCOORD }
    } };

    pipeline = device->createPipeline(&vertexLayoutDescription, vertexShader, pixelShader);
    skyBoxPipeline = device->createPipeline(&vertexLayoutDescription, skyBoxVertexShader, skyBoxPixelShader);
    noLightsPipeline = device->createPipeline(&vertexLayoutDescription, noLightsVertexShader, noLightsPixelShader);
}

void GameScene::initResources()
{
    device->bindPipeline(pipeline);
    earthResource = sge::ResourceManager::getMgr().load<sge::ModelResource>("../Assets/liteEarthDiffuseSpecular.dae");
    earthResource.getResource<sge::ModelResource>()->setDevice(device);
    earthResource.getResource<sge::ModelResource>()->createBuffers();

	spaceShipResource = sge::ResourceManager::getMgr().load<sge::ModelResource>("../Assets/SpaceShip.dae");
	spaceShipResource.getResource<sge::ModelResource>()->setDevice(device);
	spaceShipResource.getResource<sge::ModelResource>()->createBuffers();

	moonResource = sge::ResourceManager::getMgr().load<sge::ModelResource>("../Assets/moonSphere.dae");
	moonResource.getResource<sge::ModelResource>()->setDevice(device);
	moonResource.getResource<sge::ModelResource>()->createBuffers();

    device->debindPipeline(pipeline);

    device->bindPipeline(noLightsPipeline);
    sunResource = sge::ResourceManager::getMgr().load<sge::ModelResource>("../Assets/sunSphere.dae");
    sunResource.getResource<sge::ModelResource>()->setDevice(device);
    sunResource.getResource<sge::ModelResource>()->createBuffers();
    device->debindPipeline(noLightsPipeline);

    device->bindPipeline(skyBoxPipeline);
    skyBoxResource = sge::ResourceManager::getMgr().load<sge::ModelResource>("../Assets/SkyBox.dae");
    skyBoxResource.getResource<sge::ModelResource>()->setDevice(device);
    skyBoxResource.getResource<sge::ModelResource>()->createBuffers();
    device->debindPipeline(skyBoxPipeline);

    fullScreenTarget = device->createRenderTarget(1, 1280, 720, true);
	overviewScreenTarget = device->createRenderTarget(1, 640, 720, true);
	earthScreenTarget = device->createRenderTarget(1, 640, 360, true);
	spaceShipScreenTarget = device->createRenderTarget(1, 640, 360, true);
}

void GameScene::interpolate(float alpha)
{
}