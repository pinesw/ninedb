#pragma once

#ifdef NINEDB_PROFILING

#include <tracy/Tracy.hpp>
#define ZoneLruCache ZoneScoped
#define ZonePbtReader ZoneScoped
#define ZonePbtWriter ZoneScoped
#define ZonePbtWriterN ZoneScopedN
#define ZonePbtFormat ZoneScoped
#define ZonePbtStorage ZoneScoped
#define ZonePbtIterator ZoneScoped
#define ZoneSkipList ZoneScoped
#define ZoneSkipListN ZoneScopedN
#define ZoneBuffer ZoneScoped
#define ZoneDb ZoneScoped

#else

#define ZoneLruCache
#define ZonePbtReader
#define ZonePbtWriter
#define ZonePbtWriterN
#define ZonePbtFormat
#define ZonePbtStorage
#define ZonePbtIterator
#define ZoneSkipList
#define ZoneSkipListN(x)
#define ZoneBuffer
#define ZoneDb

#endif
