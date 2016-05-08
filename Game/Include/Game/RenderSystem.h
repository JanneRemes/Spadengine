#pragma once

#include <cstddef>
#include <vector>
#include <string>

#include "Core/Math.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RenderQueue.h"

namespace sge
{
	class Window;
    class RenderComponent;
    class SpriteComponent;
    class ModelComponent;
    class TextComponent;
    class LightComponent;
    class CameraComponent;
    class Entity;
    struct Pipeline;
    struct Buffer;
    struct Shader;

	class RenderSystem
	{
	public:
        RenderSystem(Window& window);
        ~RenderSystem();

		void init();
		void deinit();

		GraphicsDevice* getDevice() const { return device; }

        // TODO should we take in entities or components? 
        void renderSprites(size_t count, Entity* sprites);
        void renderTexts(size_t count, Entity* texts);
        void renderModels(size_t count, Entity* models);
        void renderLights(size_t count, Entity* lights);

        void setRenderTargets(size_t count, RenderTarget* renderTargets);
        void setCameras(size_t count, Entity* cameras);

		void begin();
		void end();
        void present();
        void clear();

        // TODO one should not directly use these methods!
        void renderSprite(SpriteComponent* sprite);
        void renderText(TextComponent* text);
        void renderModel(ModelComponent* model);

        void setClearColor(float r, float g, float b, float a);
        void setClearColor(const math::vec4& color);

	private:
		
		RenderQueue queue;
        GraphicsDevice* device;
        math::vec4 clearColor;

        // Sprite rendering data.
        Pipeline* sprPipeline;
        Buffer* sprVertexBuffer;
        Buffer* sprVertexUniformBuffer;
        Buffer* sprPixelUniformBuffer;
        Shader* sprVertexShader;
        Shader* sprPixelShader;

        struct SprVertexUniformData
        {
            math::mat4 MVP;
        } sprVertexUniformData;

        struct SprPixelUniformData
        {
            math::vec4 color;
        } sprPixelUniformData;

        // Model rendering data.
        Buffer* modelVertexUniformBuffer;
        Buffer* modelPixelUniformBuffer;

        ///////////////////////////////
        // TODO move these to LightComponent!
        struct DirLight
        {
            sge::math::vec4 direction;
            sge::math::vec4 ambient;
            sge::math::vec4 diffuse;
            sge::math::vec4 specular;
        };

        struct PointLight
        {
            sge::math::vec4 position;
            sge::math::vec4 ambient;
            sge::math::vec4 diffuse;
            sge::math::vec4 specular;

            float constant;
            float mylinear;
            float quadratic;
            float pad;
        };
        //////////////////////////////

        struct ModelVertexUniformData
        {
            sge::math::mat4 PV;
            sge::math::mat4 M;
        } modelVertexUniformData;

        struct ModelPixelUniformData
        {
            DirLight dirLight;
            PointLight pointLights[40];
            sge::math::vec4 CamPos;
            int numofpl;
            int pad[3];
        } modelPixelUniformData;

        // Text rendering data.
        std::vector<sge::Texture*> charTextures; // TODO who deletes these?
        std::string previousText = "";

        // Global rendering data.
        std::vector<CameraComponent*> cameras;
        std::vector<LightComponent*> lights;

        bool initialized;
        bool acceptingCommands;
	};
}
