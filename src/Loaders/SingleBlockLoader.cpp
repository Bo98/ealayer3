/*
    EA Layer 3 Extractor/Decoder
    Copyright (C) 2010, Ben Moench.
    See License.txt
*/

#include "Internal.h"
#include "SingleBlockLoader.h"
#include "../Parser.h"
#include "../Parsers/ParserVersion6.h"

elSingleBlockLoader::elSingleBlockLoader() :
    m_Compression(0)
{
    return;
}

elSingleBlockLoader::~elSingleBlockLoader()
{
    return;
}

const std::string elSingleBlockLoader::GetName() const
{
    return "Single Block Header";
}

bool elSingleBlockLoader::Initialize(std::istream* Input)
{
    elBlockLoader::Initialize(Input);

    const std::streamoff StartOffset = m_Input->tellg();

    Header HeaderRead = ReadHeader(true);

    // Make sure its valid
    if (HeaderRead.Compression < 5 || HeaderRead.Compression > 7)
    {
        VERBOSE("L: single block loader incorrect because of compression");
        return false;
    }
    m_Compression = HeaderRead.Compression;

    if (HeaderRead.ChannelValue % 4 != 0)
    {
        VERBOSE("L: single block loader incorrect because of channel value");
        return false;
    }
    if (HeaderRead.TotalSamples1 != HeaderRead.TotalSamples2)
    {
        VERBOSE("L: single block loader incorrect because total samples don't equal each other");
        return false;
    }
    m_Input->seekg(0, std::ios_base::end);
    if (HeaderRead.BlockSize + 8 > m_Input->tellg())
    {
        VERBOSE("L: single block loader incorrect because of size");
        return false;
    }

    VERBOSE("L: single block loader correct");
    m_Input->clear();
    m_Input->seekg(StartOffset);
    return true;
}

elSingleBlockLoader::Header elSingleBlockLoader::ReadHeader(bool dontThrow)
{
    Header HeaderRead = {};

    uint32_t FlagsAndValue;

    m_Input->read((char*)&HeaderRead.Compression, 1);
    m_Input->read((char*)&HeaderRead.ChannelValue, 1);
    m_Input->read((char*)&HeaderRead.SampleRate, 2);
    m_Input->read((char*)&FlagsAndValue, 4);

    Swap(HeaderRead.SampleRate);
    Swap(FlagsAndValue);

    HeaderRead.Flags = FlagsAndValue >> 28;

    if (HeaderRead.Flags == 0)
    {
        HeaderRead.TotalSamples1 = FlagsAndValue;
    }
    else if (HeaderRead.Flags & 1)
    {
        if (!dontThrow)
            throw std::runtime_error("Unsupported single block flag.");
    }
    else if (HeaderRead.Flags & 2)
    {
        HeaderRead.TotalSamples1 = FlagsAndValue & 0x0FFFFFFF;

        uint32_t UnknownZero; // Always 0?
        m_Input->read((char*)&UnknownZero, 4);
        Swap(UnknownZero);
    }
    else if (HeaderRead.Flags & 4)
    {
        uint32_t Unknown = FlagsAndValue & 0x0FFFFFFF;

        // Body is not present (probably in another file)
        HeaderRead.TotalSamples1 = 0;
    }
    else if (HeaderRead.Flags & 8)
    {
        uint32_t Unknown = FlagsAndValue & 0x0FFFFFFF;

        m_Input->read((char*)&HeaderRead.TotalSamples1, 4);
        Swap(HeaderRead.TotalSamples1); 
    }

    if (!(HeaderRead.Flags & 4))
    {
        m_Input->read((char*)&HeaderRead.BlockSize, 4);
        m_Input->read((char*)&HeaderRead.TotalSamples2, 4);
        Swap(HeaderRead.BlockSize);
        Swap(HeaderRead.TotalSamples2);
    }
    else
    {
        HeaderRead.BlockSize = 0;
        HeaderRead.TotalSamples2 = 0;
    }

    return HeaderRead;
}


bool elSingleBlockLoader::ReadNextBlock(elBlock& Block)
{
    if (m_Input->eof() || m_CurrentBlockIndex)
    {
        return false;
    }

    std::streamoff Offset = m_Input->tellg();

    Header HeaderRead = ReadHeader();

    if (HeaderRead.Flags & 4)
    {
        return false;
    }

    // Now load the data
    uint32_t BlockSize = HeaderRead.BlockSize - 8;

    shared_array<uint8_t> Data(new uint8_t[BlockSize]);
    m_Input->read((char*)Data.get(), BlockSize);

    Block.Clear();
    Block.Data = Data;
    Block.SampleCount = HeaderRead.TotalSamples1;
    Block.Size = BlockSize;
    Block.Offset = Offset;

    Block.SampleRate = HeaderRead.SampleRate;
    Block.Channels = (HeaderRead.ChannelValue / 4) + 1;

    m_CurrentBlockIndex++;
    return true;
}

shared_ptr<elParser> elSingleBlockLoader::CreateParser() const
{
    switch (m_Compression)
    {
        case 5:
            return make_shared<elParser>();
        case 6:
        case 7:
            return make_shared<elParserVersion6>();
    }
    return shared_ptr<elParser>();
}

void elSingleBlockLoader::ListSupportedParsers(std::vector< std::string >& Names) const
{
    Names.push_back(make_shared<elParser>()->GetName());
    Names.push_back(make_shared<elParserVersion6>()->GetName());
    return;
}
