#include "WorkerQueue.h"

#include "BC7Compressor.h"

#include <stdlib.h>
#include <stdio.h>

#include <boost/thread/thread.hpp>

template <typename T>
static inline T max(const T &a, const T &b) {
  return (a > b)? a : b;
}

template <typename T>
static inline T min(const T &a, const T &b) {
  return (a < b)? a : b;
}

template <typename T>
static inline void clamp(T &x, const T &min, const T &max) {
  if(x < min) x = min;
  else if(x > max) x = max;
}

WorkerThread::WorkerThread(WorkerQueue * parent, uint32 idx) 
  : m_ThreadIdx(idx)
  , m_Parent(parent)
{ }

void WorkerThread::operator()() {

  if(!m_Parent) {
    fprintf(stderr, "%s\n", "Illegal worker thread initialization -- parent is NULL.");
    return;
  }

  CompressionFunc f = m_Parent->GetCompressionFunc();
  if(!f) {
    fprintf(stderr, "%s\n", "Illegal worker queue initialization -- compression func is NULL.");
    return;
  }

  while(1) {
    
    EAction action = m_Parent->AcceptThreadData(m_ThreadIdx);
    if(eAction_Quit == action) {
      break;
    }

    const uint8 *src = m_Parent->GetSrcForThread(m_ThreadIdx);
    uint8 *dst = m_Parent->GetDstForThread(m_ThreadIdx);
    (*f)(src, dst, 4 * m_Parent->GetNumBlocksForThread(m_ThreadIdx), 4);
  }

  m_Parent->NotifyWorkerFinished();

  return;
}

WorkerQueue::WorkerQueue(
  uint32 numThreads,
  uint32 jobSize,
  const uint8 *inBuf,
  uint32 inBufSz,
  CompressionFunc func,
  uint8 *outBuf
)
  : m_NumThreads(numThreads)
  , m_ActiveThreads(0)
  , m_JobSize(max(uint32(1), jobSize))
  , m_InBufSz(inBufSz)
  , m_InBuf(inBuf)
  , m_OutBuf(outBuf)
  , m_CompressionFunc(func)
{
  clamp(m_NumThreads, uint32(1), kMaxNumWorkerThreads);

#ifndef NDEBUG
  if(m_InBufSz % 64) {
    fprintf(stderr, "WorkerQueue.cpp -- WARNING: InBufSz not a multiple of 64. Are you sure that your image dimensions are correct?");
  }
#endif
}

void WorkerQueue::Run() {
  
  // Spawn a bunch of threads...
  boost::unique_lock<boost::mutex> lock(m_Mutex);
  for(int i = 0; i < m_NumThreads; i++) {
    WorkerThread t (this, i);
    m_ThreadHandles[m_ActiveThreads] = new boost::thread(t);
    m_ActiveThreads++;
  }

  // Wait for them to finish...
  while(m_ActiveThreads > 0) {
    m_CV.wait(lock);
  }

  // Join them all together..
  for(int i = 0; i < m_NumThreads; i++) {
    m_ThreadHandles[i]->join();
    delete m_ThreadHandles[i];
  }
}

void WorkerQueue::NotifyWorkerFinished() {
  {
    boost::lock_guard<boost::mutex> lock(m_Mutex);
    m_ActiveThreads--;
  }
  m_CV.notify_one();
}

WorkerThread::EAction WorkerQueue::AcceptThreadData(uint32 threadIdx) {
  if(threadIdx < 0 || threadIdx >= m_ActiveThreads) {
    return WorkerThread::eAction_Quit;
  }

  // How many blocks total do we have?
  const uint32 totalBlocks = m_InBufSz / 64;
  
  // Make sure we have exclusive access...
  boost::lock_guard<boost::mutex> lock(m_Mutex);
  
  // If we've completed all blocks, then mark the thread for 
  // completion.
  if(m_NextBlock >= totalBlocks) {
    return WorkerThread::eAction_Quit;
  }

  // Otherwise, this thread's offset is the current block...
  m_Offsets[threadIdx] = m_NextBlock;
  
  // The number of blocks to process is either the job size
  // or the number of blocks remaining.
  int blocksProcessed = min(m_JobSize, totalBlocks - m_NextBlock);
  m_NumBlocks[threadIdx] = blocksProcessed;

  // Make sure the next block is updated.
  m_NextBlock += blocksProcessed;

  return WorkerThread::eAction_DoWork;
}

const uint8 *WorkerQueue::GetSrcForThread(const int threadIdx) const {
  assert(m_Offsets[threadIdx] >= 0);
  assert(threadIdx >= 0);
  assert(threadIdx < m_NumThreads);

  const uint32 inBufBlockSz = 16 * 4;
  return m_InBuf + m_Offsets[threadIdx] * inBufBlockSz;
}

uint8 *WorkerQueue::GetDstForThread(const int threadIdx) const {
  assert(m_Offsets[threadIdx] >= 0);
  assert(threadIdx >= 0);
  assert(threadIdx < m_NumThreads);

  const uint32 outBufBlockSz = 16;
  return m_OutBuf + m_Offsets[threadIdx] * outBufBlockSz;
}

uint32 WorkerQueue::GetNumBlocksForThread(const int threadIdx) const {
  assert(m_Offsets[threadIdx] >= 0);
  assert(threadIdx >= 0);
  assert(threadIdx < m_NumThreads);

  return m_NumBlocks[threadIdx];
}