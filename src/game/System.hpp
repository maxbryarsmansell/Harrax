#pragma once

#include "Config.h"

#include "game/Registry.hpp"

#include "util/Log.h"

#include <cstdint>
#include <utility>
#include <vector>
#include <unordered_map>

class SystemManager;

class System
{
	friend class SystemManager;
	using EntityStore = std::vector<uint32_t>;

protected:
	EntityStore m_Entities;
};

class SystemManager
{
	friend class Registry;
	template<typename> friend class SystemRegisterer;

	using SystemStore = std::vector<std::unique_ptr<System>>;
	using SystemRegistry = std::unordered_map<const char*, uint32_t>;

private:
	SystemManager() = default;

	template<class Sys, typename ...Args>
	Sys *Create(const Args&&... args)
	{
		uint32_t systemId = static_cast<uint32_t>(m_Systems.size());
		m_Systems.emplace_back(nullptr);
		m_Systems[systemId] = std::make_unique<Sys>(std::forward<Args>(args)...);
		m_Systems[systemId]->m_Entities = Registry::Get()->View(GetSystemComponentIds<Sys>());
		return static_cast<Sys*>(m_Systems[systemId].get());
	}

	template<class Sys>
	void RegisterSystem()
	{
		const char *sysName = GetSystemName<Sys>();
		if (m_SystemsRegistry.size() < k_MaxSystems)
		{
			if (m_SystemsRegistry.find(sysName) == m_SystemsRegistry.end())
			{
				m_SystemsRegistry[sysName] = static_cast<uint32_t>(m_SystemsRegistry.size());
				LOG("Registered system '%s' !", sysName);
			}
		}
		else
		{
			LOG("Failed to register system '%s'. "
			    "Increase the limit (currently %zu) !", sysName, k_MaxSystems);
		}
	}

	template<class Sys>
	uint32_t GetSystemId()
	{
		const char *sysName = GetSystemName<Sys>();
		ASSERT(m_SystemsRegistry.find(sysName) != m_SystemsRegistry.end(),
		       "System '%s' is not registered !", sysName
		);
		return m_SystemsRegistry[sysName];
	}

private:
	SystemStore m_Systems;
	SystemRegistry m_SystemsRegistry;
};

template<typename Sys>
constexpr const char *GetSystemName() { return nullptr }

template<typename Sys>
constexpr CompIds GetSystemComponentIds() { return {}; }

template<class Sys>
class SystemRegisterer
{
public:
	SystemRegisterer()
	{
		Registry::Get()->m_SystemManager.RegisterSystem<Sys>();
	}
};

#define DECL_SYSTEM(type, ...)                                                                    \
	template<> constexpr const char *GetSystemName<type>() { return #type; }                      \
	template<> CompIds GetSystemComponentIds<type>() { return GetComponentIds<__VA_ARGS__>(); }   \
	static SystemRegisterer<type> _RegisterSystem_##type;
