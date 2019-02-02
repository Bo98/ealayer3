/*
    EA Layer 3 Extractor/Decoder
    Copyright (C) 2010, Ben Moench.
    See License.txt
*/

#pragma once

#include "Internal.h"
#include "../BlockWriter.h"

class elSingleBlockWriter : public elBlockWriter
{
public:
    elSingleBlockWriter(uint8_t Flag);
    virtual ~elSingleBlockWriter();

    /**
     * Initialize the block writer with a pointer to an output stream.
     */
    virtual void Initialize(std::ostream* Output);

    /**
     * Write the next block to the output file.
     */
    virtual void WriteNextBlock(const elBlock& Block, bool LastBlock);

protected:
    uint8_t m_Flag;
};
