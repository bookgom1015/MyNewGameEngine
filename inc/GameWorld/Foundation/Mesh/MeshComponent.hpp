#pragma once

#include "GameWorld/Foundation/Core/Component.hpp"

namespace GameWorld::Foundation {
	namespace Core {
		class Actor;
	}

	namespace Mesh {
		class MeshComponent : public Foundation::Core::Component {
		public:
			MeshComponent(Common::Debug::LogFile* const pLogFile, Core::Actor* const pOwner);
			virtual ~MeshComponent();

		public:
			virtual bool OnInitialzing() override;
			virtual void OnCleaningUp() override;

			virtual bool ProcessInput(Common::Input::InputState* const pInput) override;
			virtual bool Update(float delta) override;
			virtual bool OnUpdateWorldTransform() override;

		public:
			bool LoadMesh(LPCSTR fileName, LPCSTR baseDir, LPCSTR extension);

		private:
			bool mbAddedMesh{};

			Common::Foundation::Hash mMeshHash{};
		};
	}
}