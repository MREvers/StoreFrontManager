#include "Config.h"

/*
const char * const CollectionObject::LstUniqueTraits[] = { "manaCost", "colors", "name", "power",
"toughness", "loyalty", "text" };

const char * const CollectionObject::LstNonUniqueTraits[] = { "set", "multiverseid" };
*/

Config* Config::ms_pConfig = nullptr;

Config::Config()
{
	std::ifstream file(".\\Config\\Config.xml");
	if (!file.good())
	{
		initDefaultSettings();
	}
	else
	{
		initConfigSettings(file);
	}
}


Config::~Config()
{
}

std::string Config::GetSourceFile()
{
	return ".\\Config\\" + m_szSourceFolder + "\\" + m_szSourceFile;
}

std::string Config::GetImportSourceFile()
{
	return ".\\Config\\" + m_szSourceFolder + "\\" + m_szImportSource;
}

std::string Config::GetHistoryFolderName()
{
	return m_szHistoryFolder;
}

std::string Config::GetMetaFolderName()
{
	return m_szMetaFolder;
}

std::string Config::GetImagesFolder()
{
	return ".\\Config\\" + m_szImagesFolder;
}

std::string Config::GetCollectionsDirectory()
{
	return GetCollectionsFolderName();
}

std::string Config::GetCollectionsFolderName()
{
	return m_szCollectionsFolder;
}

std::string Config::GetKeyCode(std::string aszFullKey)
{
	std::vector<std::pair<std::string, std::string>>::iterator iter_KeyMappings = m_lstKeyCodeMappings.begin();
	for (; iter_KeyMappings != m_lstKeyCodeMappings.end(); ++iter_KeyMappings)
	{
		if (iter_KeyMappings->first == aszFullKey)
		{
			return iter_KeyMappings->second;
		}
	}

	return "";
}
std::string Config::GetFullKey(std::string aszKeyCode)
{
	std::vector<std::pair<std::string, std::string>>::iterator iter_KeyMappings = m_lstKeyCodeMappings.begin();
	for (; iter_KeyMappings != m_lstKeyCodeMappings.end(); ++iter_KeyMappings)
	{
		if (iter_KeyMappings->second == aszKeyCode)
		{
			return iter_KeyMappings->first;
		}
	}

	return "";
}

std::vector<std::pair<std::string, std::string>>& Config::GetPairedKeysList()
{
	return m_lstPairedKeys;
}
std::vector < std::string>& Config::GetIdentifyingAttributes()
{
	return m_lstIdentifyingAttributes;
}
std::vector<std::string>& Config::GetStaticAttributes()
{
	return m_lstStaticAttributes;
}

std::vector<std::string>& Config::GetPerCollectionMetaTags()
{
	return m_lstPerCollectionMetaTags;
}

bool Config::IsIdentifyingAttributes(std::string aszAttrs)
{
	return List_Find(aszAttrs, m_lstIdentifyingAttributes) != -1;
}

bool Config::IsPairedKey(std::string aszKey)
{
	return List_Find(aszKey, m_lstPairedKeys) != -1;
}

bool Config::IsValidKey(std::string aszKey)
{
	// A valid key is either a static attr or identifying attr
	return List_Find(aszKey, m_lstStaticAttributes) != -1 || List_Find(aszKey, m_lstIdentifyingAttributes) != -1;
}

bool Config::IsStaticAttribute(std::string aszAttr)
{
	return List_Find(aszAttr, m_lstStaticAttributes) != -1;
}

template<typename T> 
int Config::List_Find(T aszFind, std::vector<T>& alstFindList)
{
	std::vector<T>::iterator iter_list = alstFindList.begin();
	int index = 0;
	for (; iter_list != alstFindList.end(); iter_list++)
	{
		if (*iter_list == aszFind)
		{
			return index;
		}
		index++;
	}
	return -1;
}

int Config::List_Find(std::string aszFind, std::vector<std::pair<std::string, std::string>>& alstFindList)
{
	std::vector<std::pair<std::string, std::string>>::iterator iter_list = alstFindList.begin();
	int index = 0;
	for (; iter_list != alstFindList.end(); iter_list++)
	{
		if (iter_list->first == aszFind || iter_list->second == aszFind)
		{
			return index;
		}
		index++;
	}
	return -1;
}

Config* Config::GetConfigClass()
{
	if (ms_pConfig == nullptr)
	{
		ms_pConfig = new Config();
	}

	return ms_pConfig;
}

void Config::initDefaultSettings()
{
	m_lstStaticAttributes.push_back("manaCost");
	m_lstStaticAttributes.push_back("colors");
	m_lstStaticAttributes.push_back("name");
	m_lstStaticAttributes.push_back("power");
	m_lstStaticAttributes.push_back("toughness");
	m_lstStaticAttributes.push_back("loyalty");
	m_lstStaticAttributes.push_back("text");
	m_lstStaticAttributes.push_back("cmc");
	m_lstStaticAttributes.push_back("colorIdentity");

	m_lstIdentifyingAttributes.push_back("set");
	m_lstIdentifyingAttributes.push_back("multiverseid");

	m_lstPairedKeys.push_back(std::make_pair("set", "multiverseid"));

	m_lstKeyCodeMappings.push_back(std::make_pair("set", "set"));
	m_lstKeyCodeMappings.push_back(std::make_pair("multiverseid", "mid"));
	m_lstKeyCodeMappings.push_back(std::make_pair("manaCost", "man"));
	m_lstKeyCodeMappings.push_back(std::make_pair("colors", "clr"));
	m_lstKeyCodeMappings.push_back(std::make_pair("name", "nam"));
	m_lstKeyCodeMappings.push_back(std::make_pair("toughness", "tuf"));
	m_lstKeyCodeMappings.push_back(std::make_pair("loyalty", "loy"));
	m_lstKeyCodeMappings.push_back(std::make_pair("text", "txt"));
	m_lstKeyCodeMappings.push_back(std::make_pair("cmc", "cmc"));
	m_lstKeyCodeMappings.push_back(std::make_pair("colorIdentity", "cid"));

	m_lstPerCollectionMetaTags.push_back("Generalization");

	m_szImportSource = "AllSets.json";
	m_szSourceFile = "MtGSource.xml";
	m_szSourceFolder = "Source";

	m_szCollectionsFolder = "Collections";
	m_szHistoryFolder = "Data";
	m_szMetaFolder = "Data";
}

void Config::initConfigSettings(std::ifstream& asConfig)
{
	rapidxml::xml_document<> doc;
	std::stringstream buffer;
	buffer << asConfig.rdbuf();
	asConfig.close();

	std::cout << "Parsing Doc" << std::endl;
	std::string content(buffer.str());
	doc.parse<0>(&content[0]);
	std::cout << "Parse Done" << std::endl;

	rapidxml::xml_node<> *xmlNode_Config = doc.first_node();

	rapidxml::xml_node<> *xmlNode_Source = xmlNode_Config->first_node("Source");
	rapidxml::xml_node<> *xmlNode_ImportDefinitions = xmlNode_Source->first_node("ImportDefinitions");
	rapidxml::xml_node<> *xmlNode_ValidKeys = xmlNode_ImportDefinitions->first_node("ValidKeys");
	rapidxml::xml_node<> *xmlNode_ValidKey = xmlNode_ValidKeys->first_node("Key");
	while (xmlNode_ValidKey != 0)
	{
		std::string szKey = xmlNode_ValidKey->value();
		m_lstStaticAttributes.push_back(szKey);

		xmlNode_ValidKey = xmlNode_ValidKey->next_sibling();
	}

	rapidxml::xml_node<> *xmlNode_IdentifyingKeys = xmlNode_ImportDefinitions->first_node("IdentifyingKeys");
	xmlNode_ValidKey = xmlNode_IdentifyingKeys->first_node("Key");
	while (xmlNode_ValidKey != 0)
	{
		std::string szKey = xmlNode_ValidKey->value();
		m_lstIdentifyingAttributes.push_back(szKey);

		xmlNode_ValidKey = xmlNode_ValidKey->next_sibling();
	}

	rapidxml::xml_node<> *xmlNode_PairedKeys = xmlNode_ImportDefinitions->first_node("PairedKeys");
	rapidxml::xml_node<> *xmlNode_Pair = xmlNode_PairedKeys->first_node("Pair");
	while (xmlNode_Pair != 0)
	{
		rapidxml::xml_node<> *xmlNode_Key = xmlNode_Pair->first_node();

		std::string szKey = xmlNode_Key->value();

		xmlNode_Key = xmlNode_Key->next_sibling();

		std::string szValue = xmlNode_Key->value();

		m_lstPairedKeys.push_back(std::make_pair(szKey, szValue));

		xmlNode_Pair = xmlNode_Pair->next_sibling();
	}

	rapidxml::xml_node<> *xmlNode_CodeKeys = xmlNode_ImportDefinitions->first_node("CodeKeys");
	xmlNode_Pair = xmlNode_CodeKeys->first_node("Pair");
	while (xmlNode_Pair != 0)
	{
		rapidxml::xml_node<> *xmlNode_Key = xmlNode_Pair->first_node();

		std::string szKey = xmlNode_Key->value();

		xmlNode_Key = xmlNode_Key->next_sibling();

		std::string szValue = xmlNode_Key->value();

		m_lstKeyCodeMappings.push_back(std::make_pair(szKey, szValue));

		xmlNode_Pair = xmlNode_Pair->next_sibling();
	}

	rapidxml::xml_node<> *xmlNode_Collections = xmlNode_Config->first_node("Collection");
	rapidxml::xml_node<> *xmlNode_CollectionDefinitions = xmlNode_Collections->first_node("CollectionDefinitions");
	rapidxml::xml_node<> *xmlNode_CollectionMetaKeys = xmlNode_CollectionDefinitions->first_node("CollectionMetaKeys");

	rapidxml::xml_node<> *xmlNode_MetaKey = xmlNode_CollectionMetaKeys->first_node("Key");
	while (xmlNode_MetaKey != 0)
	{
		std::string szKey = xmlNode_MetaKey->value();
		m_lstPerCollectionMetaTags.push_back(szKey);

		xmlNode_MetaKey = xmlNode_MetaKey->next_sibling();
	}

	rapidxml::xml_node<> *xmlNode_Node = xmlNode_Source->first_node("SourceFolder");
	m_szSourceFolder = xmlNode_Node->value();

	xmlNode_Node = xmlNode_Source->first_node("ImportFile");
	m_szImportSource = xmlNode_Node->value();

	xmlNode_Node = xmlNode_Source->first_node("SourceFile");
	m_szSourceFile = xmlNode_Node->value();

	xmlNode_Node = xmlNode_Collections->first_node("CollectionFolder");
	m_szCollectionsFolder = xmlNode_Node->value();

	xmlNode_Node = xmlNode_Collections->first_node("HistoryFolder");
	m_szHistoryFolder = xmlNode_Node->value();

	xmlNode_Node = xmlNode_Collections->first_node("MetaFolder");
	m_szMetaFolder = xmlNode_Node->value();

	xmlNode_Node = xmlNode_Config->first_node("Images");
	xmlNode_Node = xmlNode_Node->first_node("ImagesFolder");
	m_szImagesFolder = xmlNode_Node->value();
}

