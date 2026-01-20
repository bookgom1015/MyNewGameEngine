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
			virtual ~MeshComponent() = default;

		public:
			virtual BOOL OnInitialzing() override;
			virtual void OnCleaningUp() override;

			virtual BOOL ProcessInput(Common::Input::InputState* const pInput) override;
			virtual BOOL Update(FLOAT delta) override;
			virtual BOOL OnUpdateWorldTransform() override;

		public:
			BOOL LoadMesh(LPCSTR fileName, LPCSTR baseDir, LPCSTR extension);

		private:
			BOOL mbAddedMesh{};

			Common::Foundation::Hash mMeshHash{};
		};
	}
}