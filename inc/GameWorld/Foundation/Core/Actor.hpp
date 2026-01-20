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
			const std::string& name,
			DirectX::XMFLOAT3 pos = { 0.f, 0.f, 0.f },
			DirectX::XMFLOAT4 rot = { 0.f, 0.f, 0.f, 1.f },
			DirectX::XMFLOAT3 scale = { 1.f, 1.f, 1.f });
		Actor(
			Common::Debug::LogFile* const pLogFile, 
			const std::string& name, 
			const Common::Foundation::Mesh::Transform& trans);
		virtual ~Actor();

	public:
		__forceinline constexpr const std::string& Name() const;
		__forceinline constexpr const Common::Foundation::Mesh::Transform& GetTransform() const;

		__forceinline constexpr BOOL Initialized() const;
		__forceinline constexpr BOOL IsDead() const;

	protected:
		virtual BOOL OnInitialzing();
		virtual BOOL ProcessActorInput(Common::Input::InputState* const pInputState);
		virtual BOOL UpdateActor(FLOAT delta);

	private:
		BOOL UpdateComponents(FLOAT delta);
		BOOL OnUpdateWorldTransform();

	public:
		BOOL Initialize();
		void CleanUp();

		BOOL ProcessInput(Common::Input::InputState* const pInputState);
		BOOL Update(FLOAT delta);

	public: // Associated with components
		void AddComponent(Component* const pComponent);
		void RemoveComponent(Component* const pComponent);

	public: // Controlling transform functions
		void SetPosition(const DirectX::XMFLOAT3& pos);
		void SetPosition(const DirectX::XMVECTOR& pos);
		void AddPosition(const DirectX::XMFLOAT3& pos);
		void AddPosition(const DirectX::XMVECTOR& pos);

		void SetRotation(const DirectX::XMFLOAT4& rot);
		void SetRotation(const DirectX::XMVECTOR& rot);
		void AddRotation(const DirectX::XMFLOAT4& rot);
		void AddRotation(const DirectX::XMVECTOR& rot);

		void AddRotationPitch(FLOAT rad);
		void AddRotationYaw(FLOAT rad);
		void AddRotationRoll(FLOAT rad);

		void SetScale(const DirectX::XMFLOAT3& scale);
		void SetScale(const DirectX::XMVECTOR& scale);

	protected:
		Common::Debug::LogFile* mpLogFile{};

	private:
		BOOL mbInitialized{};
		BOOL mbIsDead{};
		BOOL mbNeedToUpdate{ TRUE };

		std::string mName{};

		Common::Foundation::Mesh::Transform mTransform{};

		std::vector<std::unique_ptr<Component>> mComponents{};
	};
}

#include "Actor.inl"