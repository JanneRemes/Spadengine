#include "Renderer/Buffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RenderData.h"
#include "Renderer/RenderTarget.h"
#include "Renderer/VertexLayout.h"
#include "Renderer/Window.h"

#include "Resources/ResourceManager.h"
#include "Resources/ShaderResource.h"

#include "Game/RenderSystem.h"
#include "Game/Entity.h"

#include "Game/CameraComponent.h"
#include "Game/LightComponent.h"
#include "Game/ModelComponent.h"
#include "Game/RenderComponent.h"
#include "Game/SpriteComponent.h"
#include "Game/TextComponent.h"
#include "Game/TransformComponent.h"


namespace sge
{
    RenderSystem::RenderSystem(Window& window) :
		queue(1000),
        initialized(false),
        acceptingCommands(false)
	{
		device = new GraphicsDevice(window);
	}

    RenderSystem::~RenderSystem()
	{
		delete device;
	}

    void RenderSystem::init()
	{
		device->init();

        Handle<ShaderResource> pixelShaderHandle;
        Handle<ShaderResource> vertexShaderHandle;

#ifdef DIRECTX11
        vertexShaderHandle = ResourceManager::getMgr().load<ShaderResource>("../../Shaders/Compiled/SimpleVertexShader.cso");
        pixelShaderHandle = ResourceManager::getMgr().load<ShaderResource>("../../Shaders/Compiled/SimplePixelShader.cso");
#elif OPENGL4
        vertexShaderHandle = ResourceManager::getMgr().load<ShaderResource>("../../Shaders/Compiled/SimpleVertexShader.glsl");
        pixelShaderHandle = ResourceManager::getMgr().load<ShaderResource>("../../Shaders/Compiled/SimplePixelShader.glsl");
#endif

        sge::VertexLayoutDescription vertexLayoutDescription = { 2,
        {
            { 0, 3, sge::VertexSemantic::POSITION },
            { 0, 2, sge::VertexSemantic::TEXCOORD }
        } };

        float vertexData[] = {
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,

            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f
        };

        const std::vector<char>& vShaderData = vertexShaderHandle.getResource<ShaderResource>()->loadShader();
        const std::vector<char>& pShaderData = pixelShaderHandle.getResource<ShaderResource>()->loadShader();

        sprVertexShader = device->createShader(sge::ShaderType::VERTEX, vShaderData.data(), vShaderData.size());
        sprPixelShader = device->createShader(sge::ShaderType::PIXEL, pShaderData.data(), pShaderData.size());

        sprPipeline = device->createPipeline(&vertexLayoutDescription, sprVertexShader, sprPixelShader);
        sprVertexBuffer = device->createBuffer(sge::BufferType::VERTEX, sge::BufferUsage::DYNAMIC, sizeof(vertexData));
        sprVertexUniformBuffer = device->createBuffer(sge::BufferType::UNIFORM, sge::BufferUsage::DYNAMIC, sizeof(sprVertexUniformData));
        sprPixelUniformBuffer = device->createBuffer(sge::BufferType::UNIFORM, sge::BufferUsage::DYNAMIC, sizeof(sprPixelUniformData));

        device->bindPipeline(sprPipeline);
        device->bindVertexBuffer(sprVertexBuffer);
        device->copyData(sprVertexBuffer, sizeof(vertexData), vertexData);
        device->debindPipeline(sprPipeline);

        initialized = true;
	}

    void RenderSystem::deinit()
	{
        device->deleteShader(sprVertexShader);
        device->deleteShader(sprPixelShader);
        device->deleteBuffer(sprVertexBuffer);
        device->deleteBuffer(sprVertexUniformBuffer);
        device->deleteBuffer(sprPixelUniformBuffer);
        device->deletePipeline(sprPipeline);

		device->deinit();

        initialized = false;
	}

    void RenderSystem::renderSprites(size_t count, Entity* sprites)
    {
        SGE_ASSERT(acceptingCommands);

        for (size_t i = 0; i < count; i++)
        {
            SpriteComponent* sprite = sprites[i].getComponent <SpriteComponent>();

            if (sprite)
            {
                sprite->setRenderer(this);

                for (auto camera : cameras)
                {
                    queue.push(sprite->key, std::bind(&SpriteComponent::render, sprite, std::placeholders::_1));
                }
            }
        }
    }

    void RenderSystem::renderTexts(size_t count, Entity* texts)
    {
        SGE_ASSERT(acceptingCommands);

        for (size_t i = 0; i < count; i++)
        {
            TextComponent* text = texts[i].getComponent <TextComponent>();

            if (text)
            {
                text->setRenderer(this);

                for (auto camera : cameras)
                {
                    queue.push(text->key, std::bind(&TextComponent::render, text, std::placeholders::_1));
                }
            }
        }
    }

    void RenderSystem::renderModels(size_t count, Entity* models)
    {
        SGE_ASSERT(acceptingCommands);
    }

    void RenderSystem::renderLights(size_t count, Entity* lights)
    {
        SGE_ASSERT(acceptingCommands);

        this->lights.clear();

        for (size_t i = 0; i < count; i++)
        {
            LightComponent* light = lights[i].getComponent<LightComponent>();

            SGE_ASSERT(light);

            this->lights.push_back(light);
        }
    }

    void RenderSystem::setRenderTargets(size_t count, RenderTarget* renderTargets)
    {
        SGE_ASSERT(acceptingCommands);
    }

    void RenderSystem::setCameras(size_t count, Entity* cameras)
    {
        SGE_ASSERT(acceptingCommands);
        
        this->cameras.clear();

        for (size_t i = 0; i < count; i++)
        {
            CameraComponent* camera = cameras[i].getComponent<CameraComponent>();

            SGE_ASSERT(camera);

            this->cameras.push_back(camera);
        }
    }

    void RenderSystem::begin()
    {
        SGE_ASSERT(initialized && !acceptingCommands);

        queue.begin();

        acceptingCommands = true;
    }

    void RenderSystem::end()
	{
        SGE_ASSERT(acceptingCommands);

		queue.end();

        acceptingCommands = false;

        for (auto& command : queue.getQueue())
        {
            command.second(device);
        }
	}

    void RenderSystem::present()
    {
        SGE_ASSERT(initialized && !acceptingCommands);

        device->swap();
        device->clear(0.5f, 0.6f, 0.2f, 1.0f);
    }

    void RenderSystem::clear()
    {
        SGE_ASSERT(initialized && !acceptingCommands);

        queue.clear();
        cameras.clear();
        lights.clear();   
    }

    void RenderSystem::renderSprite(SpriteComponent* sprite)
    {
        static size_t pass = 0;

        device->bindPipeline(sprPipeline);

        sge::Texture* texture = sprite->getTexture();

        if (texture)
        {
            device->bindTexture(texture, 0);
        }

        device->bindViewport(cameras[pass]->getViewport());

        sprVertexUniformData.MVP = cameras[pass]->getViewProj() * sprite->getComponent<TransformComponent>()->getMatrix();
        sprPixelUniformData.color = sprite->getColor();

        device->bindVertexUniformBuffer(sprVertexUniformBuffer, 0);
        device->copyData(sprVertexUniformBuffer, sizeof(sprVertexUniformData), &sprVertexUniformData);

        device->bindPixelUniformBuffer(sprPixelUniformBuffer, 1);
        device->copyData(sprPixelUniformBuffer, sizeof(sprPixelUniformData), &sprPixelUniformData);

        device->draw(6);

        if (texture)
        {
            device->debindTexture(texture, 0);
        }

        device->debindPipeline(sprPipeline);

        if (++pass >= cameras.size())
            pass = 0;
    }

    void RenderSystem::renderText(TextComponent* text)
    {
        // TODO text rendering uses sprite pipeline. Change if necessary.
        device->bindPipeline(sprPipeline);

        sge::Font* font = text->getFont();
        FT_GlyphSlot slot = font->face->glyph;

        // Updates textures if text has changed since previous rendering
        if (text->getText() != previousText)
        {
            while (charTextures.size() != 0)
            {
                charTextures.erase(charTextures.begin());
            }

            for (size_t i = 0; i < text->getText().size(); i++)
            {
                FT_Load_Char(font->face, text->getText()[i], FT_LOAD_RENDER);

                unsigned char* expandedData = new unsigned char[2 * slot->bitmap.width * slot->bitmap.rows];
                for (size_t j = 0; j < slot->bitmap.rows; j++)
                {
                    for (size_t k = 0; k < slot->bitmap.width; k++)
                    {
                        expandedData[2 * (k + j * slot->bitmap.width)] = 255;
                        expandedData[2 * (k + j * slot->bitmap.width) + 1] = (k >= slot->bitmap.width || j >= slot->bitmap.rows) ? 0 : slot->bitmap.buffer[k + slot->bitmap.width * j];
                    }
                }

                sge::Texture* texture = device->createTextTexture(slot->bitmap.width, slot->bitmap.rows, expandedData);

                charTextures.push_back(texture);
                delete[] expandedData;
            }
            previousText = text->getText();
        }

        // TODO added support for multiple cameras.
        static size_t pass = 0;

        // Render text
        sge::math::vec2 pen = { 0, 0 };
        sge::math::vec3 originalPosition = text->getParent()->getComponent<TransformComponent>()->getPosition();
        sge::math::vec3 originalScale = text->getParent()->getComponent<TransformComponent>()->getScale();
        for (size_t i = 0; i < text->getText().size(); i++)
        {
            FT_Load_Char(font->face, text->getText()[i], FT_LOAD_RENDER);

            sge::Texture* texture = charTextures[i];

            if (texture)
            {
                device->bindTexture(texture, 0);
            }

            pen.y = slot->metrics.vertBearingY / 32 - font->characterSize;

            if (slot->metrics.height / 64 - slot->metrics.horiBearingY / 64 > 0)
            {
                pen.y = slot->metrics.height / 64 - (float)slot->metrics.horiBearingY / 64;
            }

            text->getParent()->getComponent<TransformComponent>()->addPosition(glm::vec3(pen.x * 2, pen.y, 0));
            text->getParent()->getComponent<TransformComponent>()->setScale(originalScale * sge::math::vec3(slot->bitmap.width, slot->bitmap.rows, 1));

            device->bindViewport(cameras[pass]->getViewport());

            sprVertexUniformData.MVP = cameras[pass]->getViewProj() * text->getComponent<TransformComponent>()->getMatrix();
            sprPixelUniformData.color = text->getColor();

            device->bindVertexUniformBuffer(sprVertexUniformBuffer, 0);
            device->copyData(sprVertexUniformBuffer, sizeof(sprVertexUniformData), &sprVertexUniformData);
            device->bindPixelUniformBuffer(sprPixelUniformBuffer, 1);
            device->copyData(sprPixelUniformBuffer, sizeof(sprPixelUniformData), &sprPixelUniformData);

            device->draw(6);

            if (texture)
            {
                device->debindTexture(texture, 0);
            }

            pen.x = (float)(slot->advance.x >> 6);
        }

        text->getComponent<TransformComponent>()->setPosition(originalPosition);
        text->getComponent<TransformComponent>()->setScale(originalScale);
        device->debindPipeline(sprPipeline);

        if (++pass >= cameras.size())
            pass = 0;
    }

    void RenderSystem::renderModel(ModelComponent* model)
    {

    }

}