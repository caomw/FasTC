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

#include "FasTC/ImageFile.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cassert>
#include <algorithm>

#include "FasTC/ImageWriter.h"
#include "FasTC/ImageLoader.h"
#include "FasTC/CompressedImage.h"
#include "FasTC/Image.h"
#include "FasTC/FileStream.h"

#ifdef PNG_FOUND
#  include "ImageLoaderPNG.h"
#  include "ImageWriterPNG.h"
#endif

#ifdef PVRTEXLIB_FOUND
#  include "ImageLoaderPVR.h"
#endif

#include "ImageLoaderTGA.h"
#include "ImageLoaderASTC.h"

#include "ImageLoaderKTX.h"
#include "ImageWriterKTX.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
// Static helper functions
//
//////////////////////////////////////////////////////////////////////////////////////////

static inline void ReportError(const CHAR *msg) {
  fprintf(stderr, "ImageFile -- %s\n", msg);
}

template <typename T>
static inline T abs(const T &a) {
  return a > 0? a : -a;
}

//!HACK!
#ifdef _MSC_VER
#define strncpy strncpy_s
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//
// ImageFile implementation
//
//////////////////////////////////////////////////////////////////////////////////////////

ImageFile::ImageFile(const CHAR *filename)
  : m_FileFormat(  DetectFileFormat(filename) )
  , m_FileData(NULL)
  , m_FileDataSz(-1)
  , m_Image(NULL)
{ 
  strncpy(m_Filename, filename, kMaxFilenameSz);
}

ImageFile::ImageFile(const CHAR *filename, EImageFileFormat format)
  : m_FileFormat(format)
  , m_FileData(NULL)
  , m_FileDataSz(-1)
  , m_Image(NULL)
{ 
  strncpy(m_Filename, filename, kMaxFilenameSz);
}

ImageFile::ImageFile(const char *filename, EImageFileFormat format, const FasTC::Image<> &image)
  : m_FileFormat(format)
  , m_FileData(NULL)
  , m_FileDataSz(-1)
  , m_Image(image.Clone())
{
  strncpy(m_Filename, filename, kMaxFilenameSz);
}

ImageFile::~ImageFile() { 
  if(m_Image) {
    delete m_Image;
    m_Image = NULL;
  }

  if(m_FileData) {
    delete [] m_FileData;
    m_FileData = NULL;
  }
}

bool ImageFile::Load() {

  if(m_Image) {
    delete m_Image;
    m_Image = NULL;
  }
  
  if(m_FileData) {
    delete [] m_FileData;
    m_FileData = NULL;
  }

  if(ReadFileData(m_Filename)) {
    m_Image = LoadImage();
  }

  return m_Image != NULL;
}

bool ImageFile::Write() {

  ImageWriter *writer = NULL;
  switch(m_FileFormat) {

#ifdef PNG_FOUND
    case eFileFormat_PNG:
      writer = new ImageWriterPNG(*m_Image);
      break;
#endif // PNG_FOUND

    case eFileFormat_KTX:
      writer = new ImageWriterKTX(*m_Image);
      break;

  default:
    fprintf(stderr, "Unable to write image: unknown file format.\n");
    return false;
  }

  if(NULL == writer)
    return false;

  if(!writer->WriteImage()) {
    delete writer;
    return false;
  }

  WriteImageDataToFile(writer->GetRawFileData(), uint32(writer->GetRawFileDataSz()), m_Filename);

  delete writer;
  return true;
}

FasTC::Image<> *ImageFile::LoadImage() const {

  ImageLoader *loader = NULL;
  switch(m_FileFormat) {

#ifdef PNG_FOUND
    case eFileFormat_PNG:
      loader = new ImageLoaderPNG(m_FileData);
      break;
#endif // PNG_FOUND

#ifdef PVRTEXLIB_FOUND
    case eFileFormat_PVR:
      loader = new ImageLoaderPVR(m_FileData);
      break;
#endif // PVRTEXLIB_FOUND

    case eFileFormat_TGA:
      loader = new ImageLoaderTGA(m_FileData, m_FileDataSz);
      break;

    case eFileFormat_KTX:
      loader = new ImageLoaderKTX(m_FileData, m_FileDataSz);
      break;

    case eFileFormat_ASTC:
      loader = new ImageLoaderASTC(m_FileData, m_FileDataSz);
      break;

    default:
      fprintf(stderr, "Unable to load image: unknown file format.\n");
      return NULL;
  }

  if(!loader)
    return NULL;

  FasTC::Image<> *i = loader->LoadImage();
  if(i == NULL) {
    fprintf(stderr, "Unable to load image!\n");
  }

  // Cleanup
  delete loader;

  return i;
}

EImageFileFormat ImageFile::DetectFileFormat(const CHAR *filename) {

  size_t len = strlen(filename);
  if(len >= 256) {
    assert(false);
    ReportError("Filename too long!");
    return kNumImageFileFormats;
  }

  size_t dotPos = len - 1;

  while((dotPos >= len)? false : filename[dotPos--] != '.');
  if (dotPos >= len) {
    assert(!"Malformed filename... no .ext");
    return kNumImageFileFormats;
  }
  
  // consume the last character...
  dotPos++;

  const CHAR *ext = &filename[dotPos];

  if(strcmp(ext, ".png") == 0) {
    return eFileFormat_PNG;
  }
  else if(strcmp(ext, ".pvr") == 0) {
    return eFileFormat_PVR;
  }
  else if(strcmp(ext, ".tga") == 0) {
    return eFileFormat_TGA;
  }
  else if(strcmp(ext, ".ktx") == 0) {
    return eFileFormat_KTX;
  }
  else if(strcmp(ext, ".astc") == 0) {
    return eFileFormat_ASTC;
  }

  return kNumImageFileFormats;
}

bool ImageFile::ReadFileData(const CHAR *filename) {
  FileStream fstr (filename, eFileMode_ReadBinary);
  if(fstr.Tell() < 0) {
    fprintf(stderr, "Error opening file for reading: %s\n", filename);
    return 0;
  }

  // Figure out the filesize.
  fstr.Seek(0, FileStream::eSeekPosition_End);
  uint32 fileSize = fstr.Tell();

  // Allocate data for file contents
  m_FileData = new unsigned char[fileSize];

  // Return stream to beginning of file
  fstr.Seek(0, FileStream::eSeekPosition_Beginning);
  assert(fstr.Tell() == 0);

  // Read all of the data
  uint64 totalBytesRead = 0;
  uint64 totalBytesLeft = fileSize;
  int32 bytesRead;
  while((bytesRead = fstr.Read(m_FileData, uint32(fileSize))) > 0) {
    totalBytesRead += bytesRead;
    totalBytesLeft -= bytesRead;
  }

  if(totalBytesRead != fileSize) {
    assert(!"We didn't read as much data as we thought we had!");
    fprintf(stderr, "Internal error: Incorrect file size assumption\n");
    return false;
  }

  m_FileDataSz = fileSize;
  return true;
}

bool ImageFile::WriteImageDataToFile(const uint8 *data,
                                     const uint32 dataSz,
                                     const CHAR *filename) {

  // Open a file stream and write out the data...
  FileStream fstr (filename, eFileMode_WriteBinary);
  if(fstr.Tell() < 0) {
    fprintf(stderr, "Error opening file for reading: %s\n", filename);
    return 0;
  }

  fstr.Write(data, dataSz);
  fstr.Flush();
  return true;
}
