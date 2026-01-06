#pragma once

#include <string>  

namespace se
{
	class Resource
	{
	public:
		Resource(const std::string GUID, const std::string Name)
		{
			this->GUID = GUID;
			this->Name = Name;
		}
		~Resource();

		const std::string& getGUID() const { return GUID; }
		const std::string& getName() const { return Name; }

		void setName(const std::string& name) { Name = name; }


	private:
		std::string GUID;
		std::string Name;

	};
} // namespace se