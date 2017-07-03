#pragma once
#include <vector>
#include <string>

#include "CollectionIO.h"
#include "CollectionSource.h"
#include "CollectionItem.h"
#include "StringHelper.h"
#include "Transaction.h"

class ItemCollection
{
public:
	ItemCollection(std::string aszName,
				   CollectionSource* aoSource,
				   std::string aszParentCollectionName = "");
	~ItemCollection();

	std::string GetName();

	void AddItem(std::string aszName,
		std::vector<Tag> alstAttrs = std::vector<Tag>(),
		std::vector<Tag> alstMetaTags = std::vector<Tag>(),
		bool abCloseTransaction = true);

	void RemoveItem(std::string aszName, std::string aszIdentifyingHash, bool abCloseTransaction = true);

	void ChangeItem(std::string aszName, std::string aszIdentifyingHash, std::vector<Tag> alstChanges, bool abCloseTransaction = true);

	void LoadCollection(std::string aszFileName);

	std::vector<std::string> GetCollectionList();

	bool IsLoaded = false;

private:
	std::string m_szName;
	std::string m_szParentName;
	CollectionSource* m_ptrCollectionSource;

	std::vector<Transaction> m_lstTransactions;

	std::vector<int> m_lstItemCacheIndexes;

	std::vector<int> getCollection();

	void addItem(std::string aszName, std::vector<Tag> alstAttrs, std::vector<Tag> alstMetaTags);
	void removeItem(std::string aszName, std::string aszIdentifyingHash);
	void changeItem(std::string aszName, std::string aszIdentifyingHash, std::vector<Tag> alstChanges);
	void registerItem(int aiCacheIndex);

	Transaction* getOpenTransaction();
	void finalizeTransaction();

	void loadPreprocessingLines(std::vector<std::string> alstLine);
	void loadPreprocessingLine(std::string aszLine);

	void loadLines(std::vector<std::string> aszLines);
	void loadInterfaceLine(std::string aszLine);

	void loadAdditionLine(std::string aszLine);
	void loadRemoveLine(std::string aszLine);
	void loadDeltaLine(std::string aszLine);

};

