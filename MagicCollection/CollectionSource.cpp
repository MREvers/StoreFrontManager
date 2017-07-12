#include "CollectionSource.h"
CollectionSource::CollectionSource()
{
	m_iAllCharBuffSize = 0;
	m_AllCharBuff = new char[4000000];
}

CollectionSource::~CollectionSource()
{

}

void CollectionSource::LoadLib(std::string aszFileName)
{
	m_lstCardBuffer.clear();
	m_lstCardBuffer.reserve(17000);

	rapidxml::xml_document<> doc;
	std::ifstream file(aszFileName);
	if (!file.good())
	{
		return;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	std::cout << "Parsing Doc" << std::endl;
	std::string content(buffer.str());
	doc.parse<0>(&content[0]);
	std::cout << "Parse Done" << std::endl;

	// Mechanisms
	clock_t begin = clock();

	rapidxml::xml_node<> *xmlNode_CardDatabase = doc.first_node();
	// With the xml example above this is the <document/> node
	rapidxml::xml_node<> *xmlNode_Cards = xmlNode_CardDatabase->first_node("cards");
	rapidxml::xml_node<> *xmlNode_Card = xmlNode_Cards->first_node();
	std::string szNameKeyCode = Config::Instance()->GetKeyCode("name");
	while (xmlNode_Card != 0)
	{
		rapidxml::xml_node<> *xmlNode_CardAttribute = xmlNode_Card->first_node();

		rapidxml::xml_node<> *xmlNode_CardName = xmlNode_Card->first_node("name");

		SourceObject* sO = new SourceObject(m_iAllCharBuffSize);
		std::string aszName = xmlNode_CardName->value();
		m_iAllCharBuffSize += sO->AddAttribute(szNameKeyCode, xmlNode_CardName->value(), m_AllCharBuff, m_iAllCharBuffSize);
		m_lstCardBuffer.push_back(*sO);
		delete sO;
		sO = &m_lstCardBuffer[m_lstCardBuffer.size() - 1];

		bool bHasAll = false;
		while (xmlNode_CardAttribute != 0)
		{
			std::string szCardKey = xmlNode_CardAttribute->name();
			std::string keyCode = Config::Instance()->GetKeyCode(szCardKey);
			if (keyCode != "" && keyCode != szNameKeyCode)
			{
				m_iAllCharBuffSize += sO->AddAttribute(
					keyCode,
					xmlNode_CardAttribute->value(),
					m_AllCharBuff,
					m_iAllCharBuffSize);
			}

			xmlNode_CardAttribute = xmlNode_CardAttribute->next_sibling();
		}

		sO->FinalizeSize();
		xmlNode_Card = xmlNode_Card->next_sibling();
	}

	finalizeBuffer();

	// Mechanisms
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	std::cout << "Load Time: " << elapsed_secs << std::endl;

	std::cout << "Load Done" << std::endl;
}

int CollectionSource::LoadCard(std::string aszCardName)
{
	int iCacheLocation = -1;
	std::string szCardName = StringHelper::Str_Trim(aszCardName, ' ');

	// Since it is faster to look in the cache, check that first.
	iCacheLocation = findInCache(szCardName, false);
	if (iCacheLocation == -1)
	{
		// Look in the Source Object Buffer for a matching item.
		// If the card is found in the buffer, create a CollectionItem and cache it.
		int iFound = findInBuffer(szCardName, false);
		if (iFound != -1)
		{
			SourceObject* oSource = &m_lstCardBuffer.at(iFound);

			std::vector<Tag> lstStaticAttrs;
			std::vector<Tag> lstAttrs = oSource->GetAttributes(m_AllCharBuff);

			std::vector<Tag>::iterator att_iter = lstAttrs.begin();
			for (; att_iter != lstAttrs.end(); ++att_iter)
			{
				lstStaticAttrs.push_back(std::make_pair(Config::Instance()->GetFullKey(att_iter->first), att_iter->second));
			}

			std::map<std::string, std::vector<std::string>> lstUnfixedAttrs = oSource->GetNonUniqueAttributeRestrictions(m_AllCharBuff);

			std::map<std::string, std::vector<std::string>> lstFixedAttrs;

			std::map<std::string, std::vector<std::string>>::iterator iter_UnfixedAttrs = lstUnfixedAttrs.begin();
			for (; iter_UnfixedAttrs != lstUnfixedAttrs.end(); ++iter_UnfixedAttrs)
			{
				lstFixedAttrs[Config::Instance()->GetFullKey(iter_UnfixedAttrs->first)] = iter_UnfixedAttrs->second;
			}

			std::vector<TraitItem> lstIdentifyingTraits;
			std::map<std::string, std::vector<std::string>>::iterator iter_Traits = lstFixedAttrs.begin();
			for (; iter_Traits != lstFixedAttrs.end(); ++iter_Traits)
			{
				TraitItem newTrait(iter_Traits->first, iter_Traits->second, Config::Instance()->GetPairedKeysList());
				lstIdentifyingTraits.push_back(newTrait);
			}

			CollectionItem oCard(aszCardName, lstStaticAttrs, lstIdentifyingTraits);

			// Store the location of the CollectionItem in the cache
			iCacheLocation = m_lstoCardCache.size();
			oSource->Cache(iCacheLocation);

			// Cache the CollectionItem
			m_lstoCardCache.push_back(oCard);
		}
	}

	return iCacheLocation;
}

CollectionItem* CollectionSource::GetCardPrototype(int aiCacheIndex)
{
	if (aiCacheIndex < m_lstoCardCache.size())
	{
		return &m_lstoCardCache.at(aiCacheIndex);
	}
	else
	{
		return NULL;
	}
}

void CollectionSource::NotifyNeedToSync(std::string aszCollectionName)
{
	m_mapSync[aszCollectionName] = false;

	std::map<std::string, bool>::iterator iter_SyncCols = m_mapSync.begin();
	for (; iter_SyncCols != m_mapSync.end(); ++iter_SyncCols)
	{
		if (iter_SyncCols->first != aszCollectionName)
		{
			iter_SyncCols->second = true;
		}
	}
}

bool CollectionSource::IsSyncNeeded(std::string aszCollectionName)
{
	if (m_mapSync.find(aszCollectionName) != m_mapSync.end())
	{
		return m_mapSync[aszCollectionName];
	}
	else
	{
		return (m_mapSync[aszCollectionName] = true);
	}
}

std::vector<int>
CollectionSource::GetCollectionCache(std::string aszCollectionName, CollectionItemType aColItemType)
{
	std::vector<int> lstRetVal;

	for (size_t i = 0; i < m_lstoCardCache.size(); i++)
	{
		if (m_lstoCardCache[i].GetCopiesForCollection(aszCollectionName, aColItemType).size() > 0)
		{
			lstRetVal.push_back(i);
		}
	}

	return lstRetVal;
}

std::vector<CopyItem*>
CollectionSource::GetCollection(std::string aszCollectionName, CollectionItemType aColItemType)
{
	std::vector<CopyItem*> lstRetVal;

	for (size_t i = 0; i < m_lstoCardCache.size(); i++)
	{
		std::vector<CopyItem*> lstCopies = m_lstoCardCache[i].GetCopiesForCollection(aszCollectionName, aColItemType);

		std::vector<CopyItem*>::iterator iter_Copy = lstCopies.begin();
		for (; iter_Copy != lstCopies.end(); ++iter_Copy)
		{
			lstRetVal.push_back(*iter_Copy);
		}
	}

	return lstRetVal;
}

std::vector<std::string> CollectionSource::GetAllCardsStartingWith(std::string aszText)
{
	std::vector<std::string> lstCards;

	bool bActiveCache = aszText.size() > 2;

	if (!bActiveCache)
	{
		m_lstSearchCache.clear();
	}

	// Check if the search is cached already
	if (bActiveCache)
	{
		int iEnd = m_lstSearchCache.size();
		for (int i = iEnd - 1; i >= 0; i--)
		{
			std::pair<std::string, std::vector<SourceObject>>
				pairICache = m_lstSearchCache[i];
			if (pairICache.first == aszText)
			{
				std::vector<SourceObject>::iterator iter_Result = pairICache.second.begin();
				for (; iter_Result != pairICache.second.end(); ++iter_Result)
				{
					lstCards.push_back((iter_Result)->GetName(m_AllCharBuff));
				}

				return lstCards;
			}
			else if (aszText.substr(0, pairICache.first.size()) == pairICache.first)
			{
				break;
			}
			else
			{
				m_lstSearchCache.pop_back();
			}
		}
	}

	// If there are still entries in the searchCache, and we are here, then we only need to search
	//  the sublist.
	std::vector<SourceObject>* lstSearchList;
	std::vector<SourceObject> lstCache;
	if (bActiveCache && m_lstSearchCache.size() > 0)
	{
		lstSearchList = &m_lstSearchCache[m_lstSearchCache.size() - 1].second;
	}
	else
	{
		lstSearchList = &m_lstCardBuffer;
	}

	std::vector<std::string> lstStartCards;
	std::vector<std::string> lstOthers;

	std::vector<SourceObject>::iterator iter_Cards = lstSearchList->begin();
	for (; iter_Cards != lstSearchList->end(); ++iter_Cards)
	{
		std::string szCard = iter_Cards->GetName(m_AllCharBuff);
		std::transform(szCard.begin(), szCard.end(), szCard.begin(), ::tolower);
		size_t iFindIndex = 0;
		iFindIndex = szCard.find(aszText);
		if (iFindIndex != std::string::npos)
		{
			if (iFindIndex == 0)
			{
				lstStartCards.push_back(iter_Cards->GetName(m_AllCharBuff));
			}
			else
			{
				lstOthers.push_back(iter_Cards->GetName(m_AllCharBuff));
			}

			if (bActiveCache)
			{
				lstCache.push_back(*iter_Cards);
			}
		}
	}

	for (std::string sz : lstStartCards)
	{
		lstCards.push_back(sz);
	}
	for (std::string sz : lstOthers)
	{
		lstCards.push_back(sz);
	}

	if (bActiveCache)
	{
		m_lstSearchCache.push_back(std::make_pair(aszText, lstCache));
	}

	return lstCards;
}

int CollectionSource::findInBuffer(std::string aszCardName, bool abCaseSensitive)
{
	std::string szName;
	int iLength = m_lstCardBuffer.size();
	for (size_t i = 0; i < iLength; i++)
	{
		szName = m_lstCardBuffer.at(i).GetName(m_AllCharBuff);
		if (szName == aszCardName)
		{
			return i;
		}
	}

	/*
	std::string szCardNameFixed = convertToSearchString(aszCardName);
	if (!abCaseSensitive)
	{
		std::transform(szCardNameFixed.begin(), szCardNameFixed.end(), szCardNameFixed.begin(), ::tolower);
	}

	int iSize = m_lstCardBuffer.size();
	int iLeft = 0;
	int iRight = iSize;
	if (iRight < 1)
	{
		return -1;
	}

	std::string szName;
	while (iLeft <= iRight)
	{
		int middle = (iLeft + iRight) / 2;

		if (middle < 0 || middle >= iSize)
		{
			return -1;
		}

		szName = convertToSearchString(m_lstCardBuffer.at(middle).GetName(m_AllCharBuff));
		if (!abCaseSensitive)
		{
			std::transform(szName.begin(), szName.end(), szName.begin(), ::tolower);
		}

		if (szName == szCardNameFixed)
			return middle;
		else if (std::strcmp(szName.c_str(), szCardNameFixed.c_str()) > 0)
			iRight = middle - 1;
		else
			iLeft = middle + 1;
	}
	*/
	return -1;
}

int CollectionSource::findInCache(std::string aszName, bool abCaseSensitive)
{
	std::vector<CollectionItem>::iterator iter_ColObj = m_lstoCardCache.begin();
	int index = 0;
	for (; iter_ColObj != m_lstoCardCache.end(); ++iter_ColObj)
	{
		if (iter_ColObj->GetName() == aszName)
		{
			return index;
		}
		index++;
	}
	return -1;
}

std::string CollectionSource::convertToSearchString(std::string& aszSearch)
{
	std::string szRetval = "";
	for (size_t i = 0; i < aszSearch.size(); i++)
	{
		if (isSearchCharacter(aszSearch[i]))
		{
			szRetval += aszSearch[i];
		}
	}
	return szRetval;
}

bool CollectionSource::isSearchCharacter(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == ',';
}

void CollectionSource::finalizeBuffer()
{
	char* newBufferSize = new char[m_iAllCharBuffSize];
	memcpy(newBufferSize, m_AllCharBuff, m_iAllCharBuffSize);
	delete[] m_AllCharBuff;
	m_AllCharBuff = newBufferSize;
}
