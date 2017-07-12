#include "Collection.h"



Collection::Collection(std::string aszName, CollectionSource* aoSource, std::string aszFileCollection, std::string aszParentCollectionName)
{
	m_szName = aszName;
	m_szFileName = aszFileCollection;
	m_ptrCollectionSource = aoSource;
	m_szParentName = aszParentCollectionName;
	m_bRecordChanges = true;
}


Collection::~Collection()
{
}

std::string Collection::GetName()
{
	return m_szName;
}

void Collection::AddItem(std::string aszName,
	std::vector<Tag> alstAttrs,
	std::vector<Tag> alstMetaTags,
	bool abCloseTransaction)
{
	// Verify the card name entered is valid
	int iValidItem = m_ptrCollectionSource->LoadCard(aszName);
	if (iValidItem != -1)
	{
		CollectionItem* item = m_ptrCollectionSource->GetCardPrototype(iValidItem);

		// This is needed for removal
		std::string szHash = item->GetHash(m_szName, alstAttrs, alstMetaTags);

		std::function<void()> fnDo;
		fnDo = std::bind(&Collection::addItem, this, aszName, alstAttrs, alstMetaTags);

		std::function<void()> fnUndo;
		fnUndo = std::bind(&Collection::removeItem, this, aszName, szHash);

		Action action(fnDo, fnUndo);

		std::string szIdentifier = "+ " + CollectionItem::ToCardLine(m_szName, aszName, alstAttrs, alstMetaTags);
		action.SetIdentifier(szIdentifier); //"Set Meta-Tag '" + aszKey + "' to '" + aszValue + "' on " + aszLongName;// + szCard;

		Transaction* transaction = getOpenTransaction();
		transaction->AddAction(action);

		if (abCloseTransaction)
		{
			finalizeTransaction();
		}

	}
}

void Collection::RemoveItem(std::string aszName, std::string aszIdentifyingHash, bool abCloseTransaction)
{
	int iValidItem = m_ptrCollectionSource->LoadCard(aszName);
	if (iValidItem == -1) { return; }

	CollectionItem* item = m_ptrCollectionSource->GetCardPrototype(iValidItem);
	CopyItem* copy = item->FindCopyItem(aszIdentifyingHash);
	if (copy == nullptr) { return; }

	std::function<void()> fnDo;
	fnDo = std::bind(&Collection::removeItem, this, aszName, aszIdentifyingHash);

	std::vector<Tag> lstAttrs = copy->GetIdentifyingAttributes();
	std::vector<Tag> lstMetas = copy->GetMetaTags(Visible);
	std::function<void()> fnUndo;
	fnUndo = std::bind(&Collection::addItem, this, aszName, lstAttrs, lstMetas);

	Action action(fnDo, fnUndo);

	std::string szIdentifier = "- " + CollectionItem::ToCardLine(m_szName, aszName, lstAttrs, lstMetas);
	action.SetIdentifier(szIdentifier); //"Set Meta-Tag '" + aszKey + "' to '" + aszValue + "' on " + aszLongName;// + szCard;

	Transaction* transaction = getOpenTransaction();
	transaction->AddAction(action);

	if (abCloseTransaction)
	{
		finalizeTransaction();
	}
}

void Collection::ChangeItem(std::string aszName, std::string aszIdentifyingHash, std::vector<Tag> alstChanges, bool abCloseTransaction)
{
	int iValidItem = m_ptrCollectionSource->LoadCard(aszName);
	if (iValidItem != -1) { return; }

	CollectionItem* item = m_ptrCollectionSource->GetCardPrototype(iValidItem);
	CopyItem* copy = item->FindCopyItem(aszIdentifyingHash);
	if (copy == nullptr) { return; }

	std::function<void()> fnDo;
	fnDo = std::bind(&Collection::changeItem, this, aszName, aszIdentifyingHash, alstChanges);

	std::vector<Tag> lstUndoChanges;
	std::vector<Tag>::iterator iter_Changes = alstChanges.begin();
	for (; iter_Changes != alstChanges.end(); ++iter_Changes)
	{
		std::string szVal = copy->GetIdentifyingAttribute(iter_Changes->first);
		if (szVal != Config::NotFoundString)
		{
			lstUndoChanges.push_back(std::make_pair(iter_Changes->first, szVal));
			continue;
		}

		szVal = copy->GetMetaTag(iter_Changes->first, MetaTagType::Any);
		if (szVal != Config::NotFoundString)
		{
			lstUndoChanges.push_back(std::make_pair(iter_Changes->first, szVal));
		}
	}

	std::function<void()> fnUndo;
	fnUndo = std::bind(&Collection::changeItem, this, aszName, aszIdentifyingHash, lstUndoChanges);

	Action action(fnDo, fnUndo);

	std::string szIdentifier = "% " + CollectionItem::ToCardLine(m_szName, aszName, alstChanges);
	action.SetIdentifier(szIdentifier); //"Set Meta-Tag '" + aszKey + "' to '" + aszValue + "' on " + aszLongName;// + szCard;

	Transaction* transaction = getOpenTransaction();
	transaction->AddAction(action);

	if (abCloseTransaction)
	{
		finalizeTransaction();
	}
}

void  Collection::SaveCollection()
{
	saveHistory();
	
	saveMeta();

	saveCollection();
}

void Collection::LoadCollection(std::string aszFileName, CollectionFactory* aoFactory)
{
	// Used to filter out already used existing copies
	std::function<std::string(CopyItem*)> fnExtractor = [&](CopyItem* item)->std::string
	{
		if (!item->IsResidentIn(m_szName)) { return item->GetHash(); }
		else { return ""; }
	};

	CollectionIO loader;
	m_bRecordChanges = false;

	std::vector<std::string> lstLines = loader.GetFileLines(aszFileName);

	std::vector<std::string> lstPreprocessLines;
	std::vector<std::string> lstCardLines = loader.GetPreprocessLines(lstLines, lstPreprocessLines);

	loadPreprocessingLines(lstPreprocessLines);

	LoadChanges(lstCardLines);

	// Also need to load metatags...
	loadMetaTagFile();

	// Now Verify Borrowed Cards (i.e. Parent != this) that were just loaded exist.
	// Two things can happen in this case. 
	// If the claimed collection exists, then try to find the referenced item and use that instead, if that fails, delete the item.
	// If the claimed collection does not exist, then try to find an identical copy that may have been created by another collection and use that. If that fails, use the one created.
	for (size_t i = 0; i < m_lstItemCacheIndexes.size(); i++)
	{
		CollectionItem* itemPrototype = m_ptrCollectionSource->GetCardPrototype(m_lstItemCacheIndexes[i]);
		std::vector<CopyItem*> lstBorrowedItems = itemPrototype->GetCopiesForCollection(m_szName, CollectionItemType::Borrowed);
		for (size_t t = 0; t < lstBorrowedItems.size(); t++)
		{
			std::string szItemParent = lstBorrowedItems[t]->GetParent();
			std::string szItemHash = lstBorrowedItems[t]->GetHash();
			if (aoFactory->CollectionExists(szItemParent)) // The aoFactory is used to check if the collection is loaded.
			{
				itemPrototype->Erase(lstBorrowedItems[t]);// This copy is erased either way. These were added as placeholders.

				std::vector<CopyItem*> existingItems = itemPrototype->FindAllCopyItems(szItemHash, szItemParent); // This list will be checked for any unused copy that matches this description.
				int iFoundAlreadyUsed = ListHelper::List_Find(szItemHash, existingItems, fnExtractor);
				if (iFoundAlreadyUsed != -1)
				{
					existingItems[iFoundAlreadyUsed]->AddResident(m_szName);
				}
			}
			else
			{ // Check if any other collection referenced the unverified copy.
				// Get a list of all other cards that supposedly belong to this collection
				std::vector<CopyItem*> lstPotentiallyAlreadyUsedItems = itemPrototype->GetCopiesForCollection(szItemParent, CollectionItemType::Local);

				int iFoundAlreadyUsed = ListHelper::List_Find(szItemHash, lstPotentiallyAlreadyUsedItems, fnExtractor);
				if (iFoundAlreadyUsed != -1)
				{
					itemPrototype->Erase(lstBorrowedItems[t]);
					lstPotentiallyAlreadyUsedItems[iFoundAlreadyUsed]->AddResident(m_szName);
				}
			}
		}
	}

	// Now this collection is COMPLETELY LOADED. Since other collections can reference this collection, without this collection being loaded,
	// those other collections may have created copies of card in this collection already; if that is the case, use those copies. Additionally,
	// check that all the copies referenced by the other collections still exist, if not, delete those copies.
	std::vector<int> lstAllPossibleCacheItems = m_ptrCollectionSource->GetCollectionCache(m_szName);
	for (size_t i = 0; i < lstAllPossibleCacheItems.size(); i++)
	{
		CollectionItem* itemPrototype = m_ptrCollectionSource->GetCardPrototype(lstAllPossibleCacheItems[i]);
		// This has to iterate over ALL cards because we don't know where dangling references are.
		std::vector<CopyItem*> lstPossibleLocals = itemPrototype->GetCopiesForCollection(m_szName, CollectionItemType::Local);
		for (size_t t = 0; t < lstPossibleLocals.size(); t++)
		{
			if (!lstPossibleLocals[t]->IsResidentIn(m_szName))
			{
				std::string szItemHash = lstPossibleLocals[t]->GetHash();
				// Duplicate duplicates because there might be a copy of an existing item.
				// The second param is empty because we want ALL items with a matching hash.
				std::vector<CopyItem*> lstDuplicateDuplicates = itemPrototype->FindAllCopyItems(szItemHash, "");

				// If there is more than one, count the number that were just added to this col, then try to find matching existing ones for each.
				// Make sure that we account for the fact that other collections can borrow up to the amount in this col.
				std::map<std::string, std::vector<CopyItem*>> mapColExistingItems;
				std::vector<CopyItem*> lstNewlyAddedItems;
				for (size_t q = 0; q < lstDuplicateDuplicates.size(); q++)
				{
					if (lstDuplicateDuplicates[q]->IsResidentIn(m_szName))
					{
						lstNewlyAddedItems.push_back(lstDuplicateDuplicates[q]);
					}
					else
					{
						std::string szTargetCol = lstDuplicateDuplicates[q]->GetParent();
						mapColExistingItems[szTargetCol].push_back(lstDuplicateDuplicates[q]);
					}
				}

				// Now go through each collection and account for each one.
				std::map<std::string, std::vector<CopyItem*>>::iterator iter_existingCol = mapColExistingItems.begin();
				for (; iter_existingCol != mapColExistingItems.end(); ++iter_existingCol)
				{
					std::vector<CopyItem*>::iterator iter_existingColItem = iter_existingCol->second.begin();
					int q = 0;
					for (; iter_existingColItem != iter_existingCol->second.end() && q < lstNewlyAddedItems.size(); ++iter_existingColItem, q++)
					{
						lstNewlyAddedItems[q]->AddResident(iter_existingCol->first);
					}

					// Delete the items unaccounted for.
					for (; q >= lstNewlyAddedItems.size() && iter_existingColItem != iter_existingCol->second.end(); ++iter_existingColItem, q++)
					{
						itemPrototype->Erase(*iter_existingColItem);
					}
				}
			}
		}
	}

	m_bRecordChanges = true;
	IsLoaded = (m_szName != Config::NotFoundString);
}

// Returns all the copies impacted by this function.
void Collection::LoadChanges(std::vector<std::string> lstLines)
{
	std::vector<std::string>::iterator iter_Lines = lstLines.begin();
	for (; iter_Lines != lstLines.end(); ++iter_Lines)
	{
		loadInterfaceLine(*iter_Lines);
	}
}

std::vector<std::string> Collection::GetCollectionList(MetaTagType atagType, bool aiCollapsed)
{
	std::function<std::string(std::pair<std::string, int>)> fnExtractor = [](std::pair<std::string, int> pVal)->std::string { return pVal.first; };
	std::vector<std::string> lstRetVal;
	std::vector<std::pair<std::string,int>> lstSeenHashes;
	std::vector<int> lstCol = getCollection();
	std::vector<int>::iterator iter_Items = lstCol.begin();
	for (; iter_Items != lstCol.end(); ++iter_Items)
	{
		CollectionItem* item = m_ptrCollectionSource->GetCardPrototype(*iter_Items);
		std::vector<CopyItem*> lstCopies = item->GetCopiesForCollection(m_szName, All);

		std::vector<CopyItem*>::iterator iter_Copy = lstCopies.begin();
		for (; iter_Copy != lstCopies.end(); ++iter_Copy)
		{
			std::string szHash = (*iter_Copy)->GetHash();
			int iCounted = ListHelper::List_Find(szHash, lstSeenHashes, fnExtractor);
			if (!aiCollapsed || (iCounted == -1))
			{
				std::string szRep = item->GetCardString(*iter_Copy, atagType, m_szName);
				lstRetVal.push_back(szRep);
				lstSeenHashes.push_back(std::make_pair(szHash, 1));
			}
			else if (iCounted != -1)
			{
				lstSeenHashes[iCounted].second++;
			}
		}
	}

	if (aiCollapsed)
	{
		std::vector<std::string> lstNewRetVal;
		for (size_t i = 0; i < lstRetVal.size(); i++)
		{
			int iCounted = lstSeenHashes[i].second;
			lstNewRetVal.push_back("x" + std::to_string(iCounted) + " " + lstRetVal[i]);
		}

		lstRetVal.clear();
		lstRetVal = lstNewRetVal;
	}

	return lstRetVal;
}

std::vector<int> Collection::getCollection()
{
	if (m_ptrCollectionSource->IsSyncNeeded(m_szName))
	{
		m_lstItemCacheIndexes = m_ptrCollectionSource->GetCollectionCache(m_szName);
	}
	return m_lstItemCacheIndexes;
}

void Collection::addItem(std::string aszName, std::vector<Tag> alstAttrs, std::vector<Tag> alstMetaTags)
{
	int iCache = m_ptrCollectionSource->LoadCard(aszName);

	CollectionItem* item = m_ptrCollectionSource->GetCardPrototype(iCache);
	item->AddCopyItem(m_szName, alstAttrs, alstMetaTags);

	registerItem(iCache);
}

void Collection::removeItem(std::string aszName, std::string aszIdentifyingHash)
{
	int iCache = m_ptrCollectionSource->LoadCard(aszName);

	CollectionItem* item = m_ptrCollectionSource->GetCardPrototype(iCache);
	item->RemoveCopyItem(aszIdentifyingHash);

	if (item->GetCopiesForCollection(m_szName, All).size() == 0)
	{
		std::vector<int> lstNewCacheIndexes;
		for (size_t i = 0; i < m_lstItemCacheIndexes.size(); i++)
		{
			if (m_lstItemCacheIndexes[i] != iCache)
			{
				lstNewCacheIndexes.push_back(m_lstItemCacheIndexes[i]);
			}
		}
		m_lstItemCacheIndexes = lstNewCacheIndexes;
	}
}

void Collection::changeItem(std::string aszName, std::string aszIdentifyingHash, std::vector<Tag> alstChanges)
{

}

void Collection::registerItem(int aiCacheIndex)
{
	int iFound = ListHelper::List_Find(aiCacheIndex, m_lstItemCacheIndexes);
	if (iFound == -1)
	{
		m_lstItemCacheIndexes.push_back(aiCacheIndex);
	}
}

Transaction* Collection::getOpenTransaction()
{
	if (m_lstTransactions.size() == 0 ||
		!m_lstTransactions.at(m_lstTransactions.size() - 1).IsOpen())
	{
		m_lstTransactions.push_back(Transaction(this));
	}

	return &m_lstTransactions.at(m_lstTransactions.size() - 1);
}

void Collection::finalizeTransaction()
{
	Transaction* transaction = getOpenTransaction();
	if (transaction->IsOpen())
	{
		transaction->Finalize(m_bRecordChanges);
	}
}

void Collection::loadMetaTagFile()
{
	// This should only be called during initial loading.
	CollectionIO ioHelper;
	std::string szFileName = ioHelper.GetMetaFile(m_szFileName);
	std::vector<std::string> lstMetaLines = ioHelper.GetFileLines(szFileName);

	for (size_t i = 0; i < lstMetaLines.size(); i++)
	{
		CollectionItem::PseudoIdentifier sudoItem;
		CollectionItem::ParseCardLine(lstMetaLines[i], sudoItem);

		std::vector<Tag> lstMetaTags = sudoItem.MetaTags;

		// Clear the meta so the hash may be obtained.
		sudoItem.MetaString = "";
		sudoItem.MetaTags.clear();

		int iRealCard = m_ptrCollectionSource->LoadCard(sudoItem.Name);
		if (iRealCard != -1)
		{
			CollectionItem* item = m_ptrCollectionSource->GetCardPrototype(iRealCard);
			std::string szPlainHash = item->GetHash(m_szName, sudoItem.Identifiers);

			// Gets the first matching item resident in this collection.
			CopyItem* matchingCopy = item->FindCopyItem(szPlainHash, m_szName);
			if (matchingCopy != nullptr)
			{
				for (size_t t = 0; t < lstMetaTags.size(); t++)
				{
					matchingCopy->SetMetaTag(lstMetaTags[t].first, lstMetaTags[t].second, MetaTagType::Public);
				}
			}
		}
	}
}

void Collection::loadPreprocessingLines(std::vector<std::string>  alstLines)
{
	std::vector<std::string>::iterator iter_Lines = alstLines.begin();
	for (; iter_Lines != alstLines.end(); ++iter_Lines)
	{
		loadPreprocessingLine(*iter_Lines);
	}
}



void Collection::loadPreprocessingLine(std::string aszLine)
{
	std::string szDefKey(Config::CollectionDefinitionKey);
	if (aszLine.size() < 2) { return; }
	if (aszLine.substr(0, szDefKey.size()) != szDefKey) { return; }

	std::string szBaseLine = aszLine.substr(2);
	std::vector<std::string> lstSplitLine = StringHelper::Str_Split(szBaseLine, "=");

	if (lstSplitLine.size() != 2) { return; }

	std::vector<std::string>::iterator iter_Lines = lstSplitLine.begin();
	for (; iter_Lines != lstSplitLine.end(); ++iter_Lines)
	{
		*iter_Lines = StringHelper::Str_Trim(*iter_Lines, ' ');
	}

	std::string szKey = lstSplitLine.at(0);
	std::string szValue = lstSplitLine.at(1);
	szValue = StringHelper::Str_Trim(szValue, '\"');

	if (szKey == "Name")
	{
		m_szName = szValue;
	}
	else if (szKey == "Parent")
	{
		m_szParentName = szValue;
	}
}

// May return null depending on input
void Collection::loadInterfaceLine(std::string aszLine)
{
	if (aszLine.size() <= 2) { return; }

	std::string szTrimmedLine = StringHelper::Str_Trim(aszLine, ' ');

	std::string szLoadDirective = szTrimmedLine.substr(0, 1);

	if (szLoadDirective == "-") // REMOVE
	{
		szTrimmedLine = szTrimmedLine.substr(1);
		// Of the form
		// Sylvan Card Name [{ set="val" color="val2" } ][: { metatag1="val" metatag2="val2" }]
		loadRemoveLine(szTrimmedLine);
	}
	else if (szLoadDirective == "%") // CHANGE
	{
		szTrimmedLine = szTrimmedLine.substr(1);
		// Of the form
		// Sylvan Card Name [{ set="val" color="val2" } ][: { metatag1="val" metatag2="val2" }] ->
		//   Another Card Name [{ set="val" color="val2" } ][: { metatag1="val" metatag2="val2" }]
		loadDeltaLine(szTrimmedLine);
	}
	else // ADD
	{
		if (szLoadDirective == "+")
		{
			szTrimmedLine = szTrimmedLine.substr(1);
		}
		// Of the form
		// Sylvan Card Name [{ set="val" color="val2" } ][: { metatag1="val" metatag2="val2" }]
		loadAdditionLine(szTrimmedLine);
	}
}

void Collection::loadAdditionLine(std::string aszLine)
{
	CollectionItem::PseudoIdentifier sudoItem;
	CollectionItem::ParseCardLine(aszLine, sudoItem);

	addItem(sudoItem.Name, sudoItem.Identifiers, sudoItem.MetaTags);
}

// This needs "Card Name : { __hash="hashval" }" All other values are irrelevant.
void Collection::loadRemoveLine(std::string aszLine)
{
	CollectionItem::PseudoIdentifier sudoItem;
	CollectionItem::ParseCardLine(aszLine, sudoItem);

	std::string szHash;
	int iHash = ListHelper::List_Find(std::string(Config::HashKey), sudoItem.MetaTags, Config::Instance()->GetTagHelper());
	if (iHash != -1)
	{
		szHash = sudoItem.MetaTags[iHash].second;
		removeItem(sudoItem.Name, szHash);
	}
}

// Sylvan Card Name [{ set="val" color="val2" } ][: { metatag1="val" metatag2="val2" }] ->
//   Another Card Name [{ set="val" color="val2" } ][: { metatag1="val" metatag2="val2" }]
void Collection::loadDeltaLine(std::string aszLine)
{
	std::vector<std::string> lstOldNew = StringHelper::Str_Split(aszLine, "->");

	CollectionItem::PseudoIdentifier sudoOldItem;
	CollectionItem::ParseCardLine(lstOldNew[0], sudoOldItem);

	CollectionItem::PseudoIdentifier sudoNewItem;
	CollectionItem::ParseCardLine(lstOldNew[1], sudoNewItem);

	std::string szHash;
	int iHash = ListHelper::List_Find(std::string(Config::HashKey), sudoOldItem.MetaTags, Config::Instance()->GetTagHelper());
	if (iHash != -1)
	{
	}

}

void Collection::saveHistory()
{
	std::vector<std::string> lstHistoryLines;
	for (size_t i = 0; i < m_lstTransactions.size(); i++)
	{
		std::vector<std::string> lstTransLines = m_lstTransactions[i].GetDescriptions();
		for (size_t t = 0; t < lstTransLines.size(); t++)
		{
			lstHistoryLines.push_back(lstTransLines[t]);
		}
	}

	if (lstHistoryLines.size() > 0)
	{
		std::string szTimeString = "";
		time_t now = time(0);
		struct tm timeinfo;
		localtime_s(&timeinfo, &now);
		char str[26];
		asctime_s(str, sizeof str, &timeinfo);
		str[strlen(str) - 1] = 0;

		CollectionIO ioHelper;
		std::ofstream oHistFile;
		oHistFile.open(Config::Instance()->GetHistoryFolderName() + "\\" + ioHelper.GetHistoryFile(m_szFileName), std::ios_base::app);

		oHistFile << "[" << str << "] " << std::endl;

		std::vector<std::string>::iterator iter_histLines = lstHistoryLines.begin();
		for (; iter_histLines != lstHistoryLines.end(); ++iter_histLines)
		{
			oHistFile << *iter_histLines << std::endl;
		}

		oHistFile.close();
	}
}

void Collection::saveMeta()
{
	std::vector<std::string> lstMetaLines = GetCollectionList(Visible);

	if (lstMetaLines.size() > 0)
	{
		CollectionIO ioHelper;
		std::ofstream oMetaFile;
		oMetaFile.open(Config::Instance()->GetMetaFolderName() + "\\"+ioHelper.GetMetaFile(m_szFileName));

		std::vector<std::string>::iterator iter_MetaLine = lstMetaLines.begin();
		for (; iter_MetaLine != lstMetaLines.end(); ++iter_MetaLine)
		{
			if (iter_MetaLine->find_first_of(':') != std::string::npos)
			{
				oMetaFile << *iter_MetaLine << std::endl;
			}
		}

		oMetaFile.close();
	}
}

void Collection::saveCollection()
{
	std::vector<std::string> lstLines = GetCollectionList(None);

	if (lstLines.size() > 0)
	{
		CollectionIO ioHelper;
		std::ofstream oColFile;
		oColFile.open(ioHelper.GetCollectionFile(m_szFileName));

		oColFile << Config::CollectionDefinitionKey << " Name=\"" << m_szName<< "\"" << std::endl;
		if (m_szParentName != "")
		{
			oColFile << Config::CollectionDefinitionKey << " Parent=\"" << m_szParentName << "\"" << std::endl;
		}

		std::vector<std::string>::iterator iter_Line = lstLines.begin();
		for (; iter_Line != lstLines.end(); ++iter_Line)
		{
			oColFile << *iter_Line << std::endl;
		}

		oColFile.close();
	}
}
