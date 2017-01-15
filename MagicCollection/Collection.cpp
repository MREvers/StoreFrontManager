#include "Collection.h"


Collection::Collection(CollectionSource* aoSource)
{
   m_ColSource = aoSource;
}


Collection::~Collection()
{
   for (auto i : m_lstCollection)
   {
      delete i;
   }
}

// Looks up from the source collection then adds it.
// Should either look up from a "source" or another collection.
// AddItem(AnotherCollection, name) -- from another collection.
// AddItem(name) -- from source
// This will also be fairly slow. In the future this should strictly be used for cards from source.
void Collection::AddItem(std::string aszNewItem)
{

   // Just use magic card shit for now

   // If this Collection doesn't already have this card...
   // Create a copy of the desired subtype. If the source has one already, this will get lost,
   //  because the pointer to it will be changed.
   CollectionObject* mCO = new MagicCardObject(aszNewItem);
   if (m_ColSource->GetCard(aszNewItem, *mCO))
   {
      mCO->IncludeInCollection(this, m_lstCollection);
   }
   
}

void Collection::AddItem(std::string aszNewItem, ICollection* aoCol)
{

}

int Collection::FindInCollection(std::string aszName)
{
   int iLeft = 0;
   int iRight = m_lstCollection.size();
   if (iRight < 1)
   {
      return -1;
   }

   while (iLeft <= iRight)
   {
      int middle = (iLeft + iRight) / 2;

      if (middle < 0 || middle >= m_lstCollection.size())
      {
         return -1;
      }

      if (m_lstCollection.at(middle)->GetName() == aszName)
         return middle;
      else if (aszName.compare(m_lstCollection.at(middle)->GetName()) < 0)
         iRight = middle - 1;
      else
         iLeft = middle + 1;
   }

   return -1;
}

bool Collection::GetFromCollection(std::string aszName, ICollectionObject& rptColO)
{
   bool bRetval = false;
   int iFound = FindInCollection(aszName);
   if (iFound != -1)
   {
      bRetval = true;
   }

   return bRetval;
}

int Collection::compareItems(std::string aszItemOne, std::string aszItemTwo)
{
   return aszItemOne.compare(aszItemTwo);
}

std::vector<ICollectionObject*> Collection::GetList()
{
   return m_lstCollection;
}