#pragma once

#include "Common/Foundation/Mesh/Transform.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"

namespace Common {
	namespace Debug {
		struct LogFile;
	}

	namespace Input {
		struct InputState;
	}
}

namespace GameWorld::Foundation::Core {
	class Actor {
	public:
		Actor(
			Common::Debug::LogFile* const pLogFile,
			const std::string& name);
		Actor(
			Common::Debug::LogFile* const pLogFile, 
			const std::string& name, 
			const Common::Foundation::Mesh::Transform& trans);
		virtual ~Actor();

	public:
		__forceinline constexpr const std::string& Name() const;
		__forceinline constexpr const Common::Foundation::Mesh::Transform& GetTransform() const;

		__forceinline constexpr bool Initialized() const;
		__forceinline constexpr bool IsDead() const;

	protected:
		virtual bool OnInitialzing();
		virtual bool ProcessActorInput(Common::Input::InputState* const pInputState);
		virtual bool UpdateActor(float delta);

	private:
		bool UpdateComponents(float delta);
		bool OnUpdateWorldTransform();

	public:
		bool Initialize();
		void CleanUp();

		virtual bool ProcessInput(Common::Input::InputState* const pInputState);
		virtual bool Update(float delta);

	public: // Associated with components
		void AddComponent(Component* const pComponent);
		void RemoveComponent(Component* const pComponent);
		
	public: // Controlling transform functions
		__forceinline void SetPosition(const DirectX::SimpleMath::Vector3& pos);
		__forceinline void AddPosition(const DirectX::SimpleMath::Vector3& pos);
		
		__forceinline void SetRotation(const DirectX::SimpleMath::Vector3& rot);
		__forceinline void AddRotation(const DirectX::SimpleMath::Vector3& rot);

		__forceinline void AddRotationPitch(float degree);
		__forceinline void AddRotationYaw(float degree);
		__forceinline void AddRotationRoll(float degree);

		__forceinline void SetScale(const DirectX::SimpleMath::Vector3& scale);

	protected:
		Common::Debug::LogFile* mpLogFile{};

	private:
		bool mbInitialized{};
		bool mbIsDead{};
		bool mbNeedToUpdate{ true };

		std::string mName{};

		Common::Foundation::Mesh::Transform mTransform{};

		std::vector<std::unique_ptr<Component>> mComponents{};
	};
}

#include "Actor.inl"