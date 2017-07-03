#include "CollectionItem.h"

CollectionItem::PseudoIdentifier::PseudoIdentifier()
{
}

CollectionItem::PseudoIdentifier::PseudoIdentifier(unsigned int aiCount, std::string aszName, std::string aszDetails)
{
	Count = aiCount;
	Name = aszName;
	DetailString = aszDetails;

	CollectionItem::ParseTagString(aszDetails, Identifiers);
}

CollectionItem::PseudoIdentifier::~PseudoIdentifier()
{
}

CollectionItem::CollectionItem(std::string aszName, std::vector<Tag> alstCommon, std::vector<TraitItem> alstRestrictions)
{
	m_szName = aszName;
	m_lstCommonTraits = alstCommon;
	m_lstIdentifyingTraits = alstRestrictions;
}


CollectionItem::~CollectionItem()
{
}

std::string CollectionItem::GetName()
{
	return m_szName;
}

CopyItem* CollectionItem::AddCopyItem(std::string aszCollectionName,
	std::vector<Tag> alstAttrs,
	std::vector<Tag> alstMetaTags)
{
	CopyItem newCopy = GenerateCopy(aszCollectionName, alstAttrs, alstMetaTags);
	m_lstCopies.push_back(newCopy);
	return &m_lstCopies.at(m_lstCopies.size() - 1);
}

CopyItem CollectionItem::GenerateCopy(std::string aszCollectionName,
	std::vector<Tag> alstAttrs,
	std::vector<Tag> alstMetaTags)
{
	CopyItem newCopy(&m_lstIdentifyingTraits, aszCollectionName, alstAttrs, alstMetaTags);

	return newCopy;
}

CopyItem* CollectionItem::FindCopyItem(std::string aszHash)
{
	std::vector<CopyItem>::iterator iter_Copies = m_lstCopies.begin();

	for (; iter_Copies != m_lstCopies.end(); ++iter_Copies)
	{
		if (iter_Copies->GetMetaTag(Config::HashKey, Hidden) == aszHash)
		{
			return (iter_Copies._Ptr);
		}
	}

	return nullptr;
}

std::vector<CopyItem*> CollectionItem::GetCopiesForCollection(std::string aszCollection, CollectionItemType aItemType)
{
	std::vector<CopyItem*> lstRetVal;
	std::vector<CopyItem>::iterator iter_Copies = m_lstCopies.begin();

	for (; iter_Copies != m_lstCopies.end(); ++iter_Copies)
	{
		if ((aItemType | Local) > 0 && iter_Copies->GetParent() == aszCollection)
		{
			lstRetVal.push_back(iter_Copies._Ptr);
		}
		else if ((aItemType | Borrowed) > 0 && ListHelper::Instance()->List_Find(aszCollection, iter_Copies->GetResidentIn()) != -1)
		{
			lstRetVal.push_back(iter_Copies._Ptr);
		}
		else if ((aItemType | Virtual) > 0 &&
			iter_Copies->GetParent() == "" &&
			ListHelper::Instance()->List_Find(aszCollection, iter_Copies->GetResidentIn()) != -1)
		{
			lstRetVal.push_back(iter_Copies._Ptr);
		}
	}

	return lstRetVal;
}

std::string CollectionItem::GetHash(std::string aszCollectionName,
	std::vector<Tag> alstAttrs,
	std::vector<Tag> alstMetaTags)
{
	CopyItem copyToHash(&m_lstIdentifyingTraits, aszCollectionName, alstAttrs, alstMetaTags);

	return copyToHash.GetHash();
}

std::string CollectionItem::GetCardString(CopyItem* aItem)
{
	return CollectionItem::ToCardLine(aItem->GetParent(), m_szName, aItem->GetIdentifyingAttributes(), aItem->GetMetaTags(Visible));
}

bool CollectionItem::ParseCardLine(std::string aszLine, PseudoIdentifier& rPIdentifier)
{
	unsigned int iCount;
	std::string szDetails;
	std::string szName;

	unsigned int i = 0;
	std::string szNum = "";
	while (i < aszLine.size() && aszLine.at(i) < '9' && aszLine.at(i) > '0')
	{
		szNum = szNum + aszLine.at(i);
		i++;
	}

	if (i >= aszLine.size())
	{
		return false;
	}

	if (szNum == "")
	{
		szNum = "1";
	}

	try
	{
		iCount = std::stoi(szNum);
	}
	catch (...)
	{
		return false;
	}

	if (aszLine.at(i) == 'x')
	{
		i++;
	}

	if (i >= aszLine.size())
	{
		return false;
	}

	szName = "";
	unsigned int iter_size = aszLine.size();
	while (i < iter_size &&
		((aszLine.at(i) >= 'a' && aszLine.at(i) <= 'z') ||
		(aszLine.at(i) >= 'A' && aszLine.at(i) <= 'Z') ||
			(aszLine.at(i) == ',' || aszLine.at(i) == ' ' || aszLine.at(i) == '-')))
	{
		szName = szName + aszLine.at(i);
		i++;
	}

	szName.erase(0, szName.find_first_not_of(' '));
	szName.erase(szName.find_last_not_of(' ') + 1);

	while (i < iter_size && (aszLine.at(i) == ' ' || aszLine.at(i) != '{'))
	{
		i++;
	}

	bool hasDets = false;
	if (i < iter_size)
	{
		if (aszLine.at(i) == '{')
		{
			hasDets = true;
		}
	}

	szDetails = "";
	if (i < iter_size && hasDets)
	{

		while (i < iter_size)
		{
			szDetails += aszLine.at(i);
			i++;
		}
	}

	// Output the details
	rPIdentifier = PseudoIdentifier(iCount, szName, szDetails);
	return true;
}

bool CollectionItem::ParseTagString(std::string aszDetails, std::vector<Tag>& rlstTags)
{
	std::vector<Tag> lstKeyVals;
	std::vector<std::string> lstDetails = StringHelper::Str_Split(aszDetails, " ");
	for (std::vector<std::string>::iterator iter_attrs = lstDetails.begin(); iter_attrs != lstDetails.end(); ++iter_attrs)
	{
		std::vector<std::string> lstPairs = StringHelper::Str_Split(*iter_attrs, "=");
		if (lstPairs.size() > 1)
		{
			std::vector<std::string> lstVal = StringHelper::Str_Split(lstPairs[1], "\"");
			if (lstVal.size() == 3)
			{
				std::string szVal = lstVal[1];
				lstKeyVals.push_back(std::make_pair(lstPairs[0], szVal));
			}
		}
	}
	rlstTags = lstKeyVals;
	return true;
}

std::string CollectionItem::ToCardLine(std::string aszParentCollection,
	std::string aszName,
	std::vector<Tag> alstAttrs,
	std::vector<Tag> alstMetaTags)
{
	std::string szLine = aszName;
	szLine += " { ";

	szLine += "Parent=\"";
	szLine += aszParentCollection;
	szLine += "\" ";

	std::vector<Tag>::iterator iter_keyValPairs;
	if (alstAttrs.size() > 0)
	{
		iter_keyValPairs = alstAttrs.begin();
		for (; iter_keyValPairs != alstAttrs.end(); ++iter_keyValPairs)
		{
			szLine += iter_keyValPairs->first;
			szLine += "=\"";
			szLine += iter_keyValPairs->second;
			szLine += "\" ";
		}

	}
	szLine += "}";

	if (alstMetaTags.size() == 0)
	{
		return szLine;
	}

	szLine += " : { ";

	iter_keyValPairs = alstMetaTags.begin();
	for (; iter_keyValPairs != alstMetaTags.end(); ++iter_keyValPairs)
	{
		szLine += iter_keyValPairs->first;
		szLine += "=\"";
		szLine += iter_keyValPairs->second;
		szLine += "\" ";
	}
	szLine += "}";

	return szLine;
}