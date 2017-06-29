#include "StoreFrontBackEnd.h"


CStoreFrontBackEnd::CStoreFrontBackEnd()
{
	// No Server for now
	m_ColSource = new CollectionSource();
	m_ColSource->LoadLib(Config::Instance()->GetSourceFile());

	m_ColFactory = new CollectionFactory(m_ColSource);
}


CStoreFrontBackEnd::~CStoreFrontBackEnd()
{
}

void CStoreFrontBackEnd::AddItem(
	std::string aszCollectionName,
	std::string aszCardNameLong,
	std::vector<std::pair<std::string, std::string>> alstMeta)
{
	std::string szName;
	unsigned int iCount;
	std::string szDetails;
	if (Collection::ParseCardLine(aszCardNameLong, iCount, szName, szDetails))
	{
		if (m_ColFactory->CollectionExists(aszCollectionName) && m_ColSource->LoadCard(szName) != -1)
		{
			Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollectionName);

			std::vector<std::pair<std::string, std::string>> lstCardAttrs;
			lstCardAttrs = Collection::ParseAttrs(szDetails);

			oCol->AddItem(szName, lstCardAttrs, alstMeta, true);
		}

	}
}

void CStoreFrontBackEnd::RemoveItem(std::string aszCollectionName,
	std::string aszCardLong,
	std::vector<std::pair<std::string, std::string>> alstMeta)
{
	if (m_ColFactory->CollectionExists(aszCollectionName))
	{
		m_ColFactory->FindOrGenerateCollection(aszCollectionName)->RemoveItem( alstMeta, aszCardLong );
	}
}

void CStoreFrontBackEnd::CreateCollection(std::string aszCollectionName)
{
   if (!m_ColFactory->CollectionExists(aszCollectionName))
   {
      Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollectionName);
      oCol->SaveCollection(oCol->GetName() + ".txt");
   }
}

void CStoreFrontBackEnd::SaveCollection(std::string aszCollectionName)
{
	if (m_ColFactory->CollectionExists(aszCollectionName))
	{
		Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollectionName);
		oCol->SaveCollection(oCol->GetName() + ".txt");
	}
}

std::vector<std::pair<std::string,std::string>> CStoreFrontBackEnd::GetCardAttributes(std::string aszCardNameLong)
{
	std::vector<std::pair<std::string, std::string>> LstRetVal;
	std::string szName;
	std::string szDetails;
	int iCount;
	// Parse the long name for the name
	if (ParseCardString(aszCardNameLong, iCount, szName, szDetails))
	{
		// Get the cache index.
		int iCacheIndex;
		if ((iCacheIndex = m_ColSource->LoadCard(szName)) != -1)
		{
			//
			std::map<std::string, std::string> mapAttrs = 
				m_ColSource->GetCardPrototype(iCacheIndex)->GetAttributesMap();
			std::map<std::string, std::string>::iterator iter_Attrs = mapAttrs.begin();
			for (; iter_Attrs != mapAttrs.end(); ++iter_Attrs)
			{
				LstRetVal.push_back(std::make_pair(iter_Attrs->first, iter_Attrs->second));
			}

			std::vector<std::pair<std::string, std::string>> LstOverwriteAttrs = ParseAttrs(szDetails);
			std::vector<std::pair<std::string, std::string>>::iterator iter_OverAttrs = LstOverwriteAttrs.begin();
			iter_Attrs = mapAttrs.begin();
			for (; iter_OverAttrs != LstOverwriteAttrs.end(); ++iter_OverAttrs)
			{
				for (; iter_Attrs != mapAttrs.end(); ++iter_Attrs)
				{
					if (iter_Attrs->first == iter_OverAttrs->first)
					{
						iter_Attrs->second = iter_OverAttrs->second;
					}
				}
			}
		}
	}
	return LstRetVal;
}

std::vector<std::string> CStoreFrontBackEnd::GetCardAttributeRestriction(std::string aszCardNameLong, std::string aszKey)
{
	std::string szCardName;
	unsigned int iAmount;
	std::string szDetails;
	if (Collection::ParseCardLine(aszCardNameLong, iAmount, szCardName, szDetails))
	{
		bool bAlreadyTagged = false;
		int iCardCacheIndex = m_ColSource->LoadCard(szCardName);
		if (iCardCacheIndex != -1)
		{
			CollectionObject* oCardClass = m_ColSource->GetCardPrototype(iCardCacheIndex);
			auto mapRestrictions = oCardClass->GetNonUniqueAttributeRestrictions();
			if (mapRestrictions.find(aszKey) != mapRestrictions.end())
			{
				return mapRestrictions[aszKey];
			}
		}
	}
	std::vector<std::string> lstNullRetVal;
	lstNullRetVal.push_back("*");
	return lstNullRetVal;
}

std::string CStoreFrontBackEnd::LoadCollection(std::string aszCollection)
{
	//std::string szFileName = Config::GetConfigClass()->GetCollectionsDirectory() + "\\" + aszCollection;
	std::string szFileName = aszCollection;
	std::ifstream f(szFileName);
	if (f.good())
	{
		// If it does, continue.
		// Check if the load is successful.
		Collection* optrLoadedCol = m_ColFactory->LoadCollectionFromFile(szFileName);
		if (optrLoadedCol != nullptr)
		{
			return optrLoadedCol->GetName();
		}
	}

	return "";
}

void CStoreFrontBackEnd::LoadBulkChanges(std::string aszCollection, std::vector<std::string> alstChanges)
{
   if (m_ColFactory->CollectionExists(aszCollection))
   {
      Collection* oColO = m_ColFactory->FindOrGenerateCollection(aszCollection);
      oColO->LoadBulkChanges(alstChanges);
   }
}

std::vector<std::string> CStoreFrontBackEnd::GetCollectionList(std::string aszCollection)
{
	if (m_ColFactory->CollectionExists(aszCollection))
	{
		return m_ColFactory->FindOrGenerateCollection(aszCollection)->GetCollectionList();
	}
	else
	{
		std::vector<std::string> lstEmpty;
		return lstEmpty;
	}
}


std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
CStoreFrontBackEnd::GetCollectionListWithMeta(std::string aszCollection)
{
	if (m_ColFactory->CollectionExists(aszCollection))
	{
		return m_ColFactory->FindOrGenerateCollection(aszCollection)->GetCollectionListWithMeta();
	}
	else
	{
		std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> lstEmpty;
		return lstEmpty;
	}
}

std::string CStoreFrontBackEnd::GetImagesPath()
{
	return Config::Instance()->GetImagesFolder();
}

std::vector<std::string> CStoreFrontBackEnd::GetLoadedCollections()
{
	return m_ColFactory->LoadedCollections;
}

std::vector<std::pair<std::string, std::string>> CStoreFrontBackEnd::ParseAttrs(std::string aszDetails)
{
	return Collection::ParseAttrs(aszDetails);
}

bool CStoreFrontBackEnd::ParseCardString(std::string aszCardString, int& riCount, std::string& rszName, std::string& rszDetails)
{
	unsigned int iNum = 0;
	std::string szName = "";
	std::string szDetails = "";
	bool bRetVal = Collection::ParseCardLine(aszCardString, iNum, szName, szDetails);

	riCount = iNum;
	rszName = szName;
	rszDetails = szDetails;

	return bRetVal;
}

std::vector<std::string> CStoreFrontBackEnd::GetAllCardsStartingWith(std::string aszText)
{
	return m_ColSource->GetAllCardsStartingWith(aszText);
}

void CStoreFrontBackEnd::SetBaselineHistory(std::string aszCollectionName)
{
   if (m_ColFactory->CollectionExists(aszCollectionName))
   {
      Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollectionName);
      oCol->CreateBaselineHistory();
   }
}

void CStoreFrontBackEnd::AddMetaTag(std::string aszCollectionName, std::string aszLongName, std::string aszKey, std::string aszValue,
	std::vector<std::pair<std::string, std::string>> alstMatchMeta)
{
	if (m_ColFactory->CollectionExists(aszCollectionName))
	{
		Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollectionName);
		oCol->SetMetaTag(aszLongName, aszKey, aszValue, alstMatchMeta);
	}
}

void  CStoreFrontBackEnd::SetMetaTags(
   std::string aszCollection,
   std::string aszLongName,
   std::vector<std::pair<std::string, std::string>> alstNewTags,
   std::vector<std::pair<std::string, std::string>> alstMatchMeta)
{
   if (m_ColFactory->CollectionExists(aszCollection))
   {
      Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollection);
      oCol->SetMetaTags(aszLongName, alstNewTags, alstMatchMeta);
   }
}

void CStoreFrontBackEnd::AddMetaTag(
	std::string aszCollectionName,
	std::string aszLongName,
	std::string aszKey,
	std::string aszSubKey,
	std::string aszValue,
	std::vector<std::pair<std::string, std::string>> alstMatchMeta)
{
	if (m_ColFactory->CollectionExists(aszCollectionName))
	{
		Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollectionName);
		oCol->SetMetaTag(aszLongName, aszKey + "." + aszSubKey, aszValue, alstMatchMeta);
	}
}

void CStoreFrontBackEnd::RemoveMetaTag(
	std::string aszCollection,
	std::string aszLongName,
	std::string aszKey,
	std::vector<std::pair<std::string, std::string>> alstMatchMeta)
{
	if (m_ColFactory->CollectionExists(aszCollection))
	{
		Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollection);
		oCol->RemoveMetaTag(aszLongName, aszKey, alstMatchMeta);
	}
}

void  CStoreFrontBackEnd::SetFeatures(
   std::string aszCollection,
   std::string aszLongName,
   std::vector<std::pair<std::string, std::string>> alstNewTags,
   std::vector<std::pair<std::string, std::string>> alstNewAttrs,
   std::vector<std::pair<std::string, std::string>> alstMatchMeta)
{
   if (m_ColFactory->CollectionExists(aszCollection))
   {
      Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollection);
      oCol->SetFeatures(aszLongName, alstNewTags, alstNewAttrs, alstMatchMeta);
   }
}

std::vector < std::vector<std::pair<std::string, std::string>>>
CStoreFrontBackEnd::GetMetaTags(std::string aszCollectionName, std::string aszLongName)
{
	if (m_ColFactory->CollectionExists(aszCollectionName))
	{
		Collection* oCol = m_ColFactory->FindOrGenerateCollection(aszCollectionName);
		return oCol->GetMetaTags(aszLongName);
	}

	std::vector < std::vector<std::pair<std::string, std::string>>> lstno;
	return lstno;
}

bool CStoreFrontBackEnd::IsSameCard(std::string aszLongOne, std::string aszLongTwo)
{
	std::string szName;
	unsigned int iAmount;
	std::string szDetails;
	if (Collection::ParseCardLine(aszLongOne, iAmount, szName, szDetails))
	{
		std::string szNameTwo;
		unsigned int iAmountTwo;
		std::string szDetailsTwo;
		if (Collection::ParseCardLine(aszLongTwo, iAmountTwo, szNameTwo, szDetailsTwo))
		{
			if (szName == szNameTwo)
			{
				int iFound = m_ColSource->LoadCard(szName);
				if (iFound != -1)
				{
					std::vector<std::pair<std::string, std::string>> lstAttrs = Collection::ParseAttrs(szDetails);
					std::vector<std::pair<std::string, std::string>> lstAttrsTwo = Collection::ParseAttrs(szDetailsTwo);
					CollectionObject* ColObject = m_ColSource->GetCardPrototype(iFound);
					CopyObject oCop = ColObject->GenerateCopy(std::string("None"), lstAttrs);
					CopyObject oCop2 = ColObject->GenerateCopy(std::string("None"), lstAttrsTwo);
					if (CollectionObject::IsSameIdentity(&oCop, &oCop2))
					{
						return true;
					}
				}

			}
		}
	}
	return false;
}

bool CStoreFrontBackEnd::IsSameMetaTags(std::vector<std::pair<std::string, std::string>> aLstOne,
	std::vector<std::pair<std::string, std::string>> aLstTwo)
{
	return CollectionObject::IsSameMetaTags(aLstOne, aLstTwo);
}

void CStoreFrontBackEnd::ImportCollection()
{
	JSONImporter JI;
	JI.ImportJSON(Config::Instance()->GetImportSourceFile());

}