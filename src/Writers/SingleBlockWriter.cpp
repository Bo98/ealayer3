/*
    EA Layer 3 Extractor/Decoder
    Copyright (C) 2010, Ben Moench.
    See License.txt
*/

#include "Internal.h"
#include "SingleBlockWriter.h"

elSingleBlockWriter::elSingleBlockWriter(uint8_t Flag) : m_Flag(Flag)
{
    return;
}

elSingleBlockWriter::~elSingleBlockWriter()
{
    return;
}

void elSingleBlockWriter::Initialize(std::ostream* Output)
{
    if (!Output)
    {
        return;
    }
    elBlockWriter::Initialize(Output);
    return;
}

void elSingleBlockWriter::WriteNextBlock(const elBlock& Block, bool LastBlock)
{
    // Sanity check
    if (!m_Output)
    {
        return;
    }

    // Calculate the variables
    uint8_t Compression;
    uint8_t ChannelValue;
    uint16_t SampleRate;
    uint32_t FlagAndValue;
    uint32_t TotalSamples;
    uint32_t BlockSize;

    Compression = 5;
    ChannelValue = Block.Channels * 4 - 4;
    SampleRate = Block.SampleRate;
    TotalSamples = Block.SampleCount;
    BlockSize = Block.Size + 8;

    FlagAndValue = m_Flag << 28;

    // Swap
    Swap(SampleRate);
    Swap(TotalSamples);
    Swap(BlockSize);

    // Write
    m_Output->write((char*)&Compression, 1);
    m_Output->write((char*)&ChannelValue, 1);
    m_Output->write((char*)&SampleRate, 2);

    if (m_Flag == 0)
    {
        m_Output->write((char*)&TotalSamples, 4);
    }
    else if (m_Flag & 1)
    {
        throw std::runtime_error("Unsupported single block flag.");
    }
    else if (m_Flag & 2)
    {
        FlagAndValue |= Block.SampleCount;
        Swap(FlagAndValue);
        uint32_t Unknown = 0;

        m_Output->write((char*)&FlagAndValue, 4);
        m_Output->write((char*)&Unknown, 4);
    }
    else if (m_Flag & 4)
    {
        FlagAndValue |= 0; // TODO
        Swap(FlagAndValue);

        m_Output->write((char*)&FlagAndValue, 4);
        // Body not present.
    }
    else if (m_Flag & 8)
    {
        FlagAndValue |= 0; // TODO
        Swap(FlagAndValue);

        m_Output->write((char*)&FlagAndValue, 4);
        m_Output->write((char*)&TotalSamples, 4);
    }

    if (!(m_Flag & 4))
    {
        m_Output->write((char*)&BlockSize, 4);
        m_Output->write((char*)&TotalSamples, 4);
        m_Output->write((char*)Block.Data.get(), Block.Size);
    }
    
    return;
}
