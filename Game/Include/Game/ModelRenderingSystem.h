#pragma once

#include "Game/System.h"
#include "Core/Math.h"
#include <vector>

namespace sge
{
    class RenderSystem;
	class ModelComponent;
    class CameraComponent;
	struct Buffer;

	class ModelRenderingSystem : public System
	{
	public:
        ModelRenderingSystem(RenderSystem* renderer);

		void renderModel(ModelComponent* model);

        void setCamera(CameraComponent* camera);

		void update();
		void addComponent(Component* component);
		
	private:
		std::vector<ModelComponent*> components;

        RenderSystem* renderer;
		
        CameraComponent* camera;

		Buffer* uniformBuffer;
		Buffer* uniformBuffer2;

		struct UniformDataComponent
		{
			sge::math::mat4 PV;
			sge::math::mat4 M;
		} uniformData;

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

		struct UniformDataComponent2
		{
			DirLight dirLight;
			PointLight pointLights[40];
			sge::math::vec4 CamPos;
            int numofpl;
			int pad[3];
		} uniformData2;
	};
}