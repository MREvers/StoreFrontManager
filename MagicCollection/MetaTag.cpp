#include "MetaTag.h"



MetaTag::MetaTag(Tag aTag, MetaTagType atagType)
{
	m_Type = atagType;
	m_Tag = aTag;
}

MetaTag::MetaTag(std::string aszKey, std::string aszVal, MetaTagType atagType) :
	MetaTag(std::make_pair(aszKey, aszVal), atagType)
{
}


MetaTag::~MetaTag()
{
}

std::string MetaTag::GetKey()
{
	return m_Tag.first;
}

std::string MetaTag::GetVal()
{
	return m_Tag.second;
}

bool MetaTag::CanView(MetaTagType atagType)
{
	return (m_Type | atagType) != 0;
}