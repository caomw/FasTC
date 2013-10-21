/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

#include "CompressedImage.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "Pixel.h"

#include "TexCompTypes.h"
#include "BC7Compressor.h"
#include "PVRTCCompressor.h"
#include "DXTCompressor.h"

CompressedImage::CompressedImage( const CompressedImage &other )
  : Image(other)
  , m_Format(other.m_Format)
  , m_CompressedData(0)
{
  if(other.m_CompressedData) {
    uint32 compressedSz = GetCompressedSize();
    m_CompressedData = new uint8[compressedSz];
    memcpy(m_CompressedData, other.m_CompressedData, compressedSz);
  }
}

CompressedImage::CompressedImage(
  const unsigned int width,
  const unsigned int height,
  const ECompressionFormat format,
  const unsigned char *data
)
  : FasTC::Image<>(width, height,
                   reinterpret_cast<uint32 *>(NULL),
                   format == eCompressionFormat_BPTC)
  , m_Format(format)
  , m_CompressedData(0)
{
  uint32 cmpSz = GetCompressedSize();
  if(cmpSz > 0) {
    assert(!m_CompressedData);
    m_CompressedData = new uint8[cmpSz];
    memcpy(m_CompressedData, data, cmpSz);
  }
}

CompressedImage &CompressedImage::operator=(const CompressedImage &other) {
  Image::operator=(other);
  m_Format = other.m_Format;
  if(other.m_CompressedData) {
    uint32 cmpSz = GetCompressedSize();
    m_CompressedData = new uint8[cmpSz];
    memcpy(m_CompressedData, other.m_CompressedData, cmpSz);
  }
  return *this;
}

CompressedImage::~CompressedImage() {
  if(m_CompressedData) {
    delete m_CompressedData;
    m_CompressedData = NULL;
  }
}

bool CompressedImage::DecompressImage(unsigned char *outBuf, unsigned int outBufSz) const {

  assert(outBufSz == GetUncompressedSize());

  uint8 *byteData = reinterpret_cast<uint8 *>(m_CompressedData);
  DecompressionJob dj (byteData, outBuf, GetWidth(), GetHeight());
  switch(m_Format) {
    case eCompressionFormat_DXT1:
      DXTC::DecompressDXT1(dj);
      break;

    case eCompressionFormat_PVRTC:
    {
#ifndef NDEBUG
      PVRTCC::Decompress(dj, false, PVRTCC::eWrapMode_Wrap, true);
#else
      PVRTCC::Decompress(dj);
#endif
    }
    break;

    case eCompressionFormat_BPTC: 
    { 
      BC7C::Decompress(dj);
    }
    break;

    default:
    {
      const char *errStr = "Have not implemented decompression method.";
      fprintf(stderr, "%s\n", errStr);
      assert(!errStr);
    }
    return false;
  }

  return true;
}

void CompressedImage::ComputePixels() {

  uint32 unCompSz = GetWidth() * GetHeight() * 4;
  uint8 *unCompBuf = new uint8[unCompSz];
  DecompressImage(unCompBuf, unCompSz);

  uint32 * newPixelBuf = reinterpret_cast<uint32 *>(unCompBuf);

  FasTC::Pixel *newPixels = new FasTC::Pixel[GetWidth() * GetHeight()];
  for(uint32 i = 0; i < GetWidth() * GetHeight(); i++) {
    newPixels[i].Unpack(newPixelBuf[i]);
  }

  SetImageData(GetWidth(), GetHeight(), newPixels);
}

uint32 CompressedImage::GetCompressedSize(uint32 uncompressedSize, ECompressionFormat format) {
  assert(uncompressedSize % 8 == 0);

  uint32 cmpDataSzNeeded = 0;
  switch(format) {
  default:
    assert(!"Not implemented!");
    // Fall through V
  case eCompressionFormat_DXT1:
  case eCompressionFormat_PVRTC:
    cmpDataSzNeeded = uncompressedSize / 8;
    break;

  case eCompressionFormat_DXT5:
  case eCompressionFormat_BPTC:
    cmpDataSzNeeded = uncompressedSize / 4;
    break;
  }

  return cmpDataSzNeeded;
}
