/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/AreaAreaIndex.h>

#include <algorithm>

#include <osmscout/util/Logger.h>

namespace osmscout {

  AreaAreaIndex::AreaAreaIndex(size_t cacheSize)
  : filepart("areaarea.idx"),
    maxLevel(0),
    topLevelOffset(0),
    indexCache(cacheSize)
  {
    // no code
  }

  void AreaAreaIndex::Close()
  {
    if (scanner.IsOpen()) {
      scanner.Close();
    }
  }

  bool AreaAreaIndex::GetIndexCell(uint32_t level,
                                   FileOffset offset,
                                   IndexCell &indexCell,
                                   FileOffset &dataOffset) const
  {
    if (level<maxLevel) {
      IndexCache::CacheRef cacheRef;

      if (!indexCache.GetEntry(offset,cacheRef)) {
        IndexCache::CacheEntry cacheEntry(offset);

        cacheRef=indexCache.SetEntry(cacheEntry);

        if (!scanner.IsOpen()) {
          if (!scanner.Open(datafilename,
                            FileScanner::LowMemRandom,
                            true)) {
            log.Error() << "Error while opening '" << scanner.GetFilename() << "' for reading!";
            return false;
          }
        }

        if (!scanner.SetPos(offset)) {
          log.Error() << "Cannot navigate to offset " << offset << " in file '" << scanner.GetFilename() << "'";
        }

        for (size_t c=0; c<4; c++) {
          FileOffset childOffset;

          if (!scanner.ReadNumber(childOffset)) {
            log.Error() << "Cannot read index data at offset " << offset << " in file '" << scanner.GetFilename() << "'";
            return false;
          }

          if (childOffset==0) {
            cacheRef->value.children[c]=0;
          }
          else {
            cacheRef->value.children[c]=offset-childOffset;
          }
        }

        if (!scanner.GetPos(cacheRef->value.data)) {
          log.Error() << "Cannot get current file position in file '" << scanner.GetFilename() << "'";
          return false;
        }

        indexCell=cacheRef->value;
      }
      else {
        indexCell=cacheRef->value;
      }
    }
    else {
      indexCell.data=offset;

      for (size_t c=0; c<4; c++) {
        indexCell.children[c]=0;
      }
    }

    dataOffset=indexCell.data;

    return true;
  }

  bool AreaAreaIndex::ReadCellData(TypeConfig& typeConfig,
                                   const TypeSet& types,
                                   FileOffset dataOffset,
                                   size_t spaceLeft,
                                   size_t currentLevel,
                                   size_t maxSizeLevel,
                                   std::vector<FileOffset>& offsets,
                                   bool& stopArea) const
  {
    if (!scanner.SetPos(dataOffset)) {
      return false;
    }

    uint32_t offsetCount;

    if (!scanner.ReadNumber(offsetCount)) {
      return false;
    }

    FileOffset prevOffset=0;

    for (size_t c=0; c<offsetCount; c++) {
      uint64_t   value;
      TypeId     typeId;
      uint8_t    sizeLevel=currentLevel;
      FileOffset areaOffset;

      if (!scanner.ReadNumber(value)) {
        return false;
      }

      if (currentLevel==maxLevel) {
        sizeLevel=value & 7;
        sizeLevel+=maxLevel;
        value=value >> 3;
      }

      typeId=value & areaTypeIdMask;
      value=value >> typeConfig.GetAreaTypeIdBits();
      areaOffset=value+prevOffset;

      prevOffset=areaOffset;

      if (sizeLevel<=maxSizeLevel &&
          types.IsTypeSet(typeId)) {
        offsets.push_back(areaOffset);

        if (offsets.size()>spaceLeft) {
          stopArea=true;
          break;
        }
      }
    }

    return true;
  }

  void AreaAreaIndex::PushCellsForNextLevel(double minlon,
                                            double minlat,
                                            double maxlon,
                                            double maxlat,
                                            const IndexCell& cellIndexData,
                                            const CellDimension& cellDimension,
                                            size_t cx,
                                            size_t cy,
                                            std::vector<CellRef>& nextCellRefs) const
  {
    if (cellIndexData.children[0]!=0) {
      // top left
      double x=cx*cellDimension.width;
      double y=(cy+1)*cellDimension.height;

      if (!(x>maxlon+cellDimension.width/2 ||
            y>maxlat+cellDimension.height/2 ||
            x+cellDimension.width<minlon-cellDimension.width/2 ||
            y+cellDimension.height<minlat-cellDimension.height/2)) {
        nextCellRefs.push_back(CellRef(cellIndexData.children[0],cx,cy+1));
      }
    }

    if (cellIndexData.children[1]!=0) {
      // top right
      double x=(cx+1)*cellDimension.width;
      double y=(cy+1)*cellDimension.height;

      if (!(x>maxlon+cellDimension.width/2 ||
            y>maxlat+cellDimension.height/2 ||
            x+cellDimension.width<minlon-cellDimension.width/2 ||
            y+cellDimension.height<minlat-cellDimension.height/2)) {
        nextCellRefs.push_back(CellRef(cellIndexData.children[1],cx+1,cy+1));
      }
    }

    if (cellIndexData.children[2]!=0) {
      // bottom left
      double x=cx*cellDimension.width;
      double y=cy*cellDimension.height;

      if (!(x>maxlon+cellDimension.width/2 ||
            y>maxlat+cellDimension.height/2 ||
            x+cellDimension.width<minlon-cellDimension.width/2 ||
            y+cellDimension.height<minlat-cellDimension.height/2)) {
        nextCellRefs.push_back(CellRef(cellIndexData.children[2],cx,cy));
      }
    }

    if (cellIndexData.children[3]!=0) {
      // bottom right
      double x=(cx+1)*cellDimension.width;
      double y=cy*cellDimension.height;

      if (!(x>maxlon+cellDimension.width/2 ||
            y>maxlat+cellDimension.height/2 ||
            x+cellDimension.width<minlon-cellDimension.width/2 ||
            y+cellDimension.height<minlat-cellDimension.height/2)) {
        nextCellRefs.push_back(CellRef(cellIndexData.children[3],cx+1,cy));
      }
    }
  }

  bool AreaAreaIndex::Load(const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
      log.Error() << "Cannot open file '" << scanner.GetFilename() << "'";
      return false;
    }

    if (!scanner.ReadNumber(maxLevel)) {
      log.Error() << "Cannot read data from file '" << scanner.GetFilename() << "'";
      return false;
    }

    if (!scanner.ReadFileOffset(topLevelOffset)) {
      log.Error() << "Cannot read data from file '" << scanner.GetFilename() << "'";
      return false;
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool AreaAreaIndex::GetOffsets(const TypeConfigRef& typeConfig,
                                 double minlon,
                                 double minlat,
                                 double maxlon,
                                 double maxlat,
                                 size_t maxLevel,
                                 const TypeSet& types,
                                 size_t maxCount,
                                 std::vector<FileOffset>& offsets) const
  {
    std::vector<CellRef>    cellRefs;     // cells to scan in this level
    std::vector<CellRef>    nextCellRefs; // cells to scan for the next level
    std::vector<FileOffset> newOffsets;   // offsets collected in the current level

    areaTypeIdMask=Pow(2,typeConfig->GetAreaTypeIdBits())-1;

    minlon+=180;
    maxlon+=180;
    minlat+=90;
    maxlat+=90;

    // Clear result data structures
    offsets.clear();

    // Make the vector preallocate memory for the expected data size
    // This should void reallocation
    offsets.reserve(std::min(20000u,(uint32_t)maxCount));
    newOffsets.reserve(std::min(20000u,(uint32_t)maxCount));

    cellRefs.reserve(2000);
    nextCellRefs.reserve(2000);

    cellRefs.push_back(CellRef(topLevelOffset,0,0));

    // For all levels:
    // * Take the tiles and offsets of the last level
    // * Calculate the new tiles and offsets that still interfere with given area
    // * Add the new offsets to the list of offsets and finish if we have
    //   reached maxLevel or maxAreaCount.
    // * copy no, ntx, nty to ctx, cty, co and go to next iteration
    bool stopArea=false;
    for (uint32_t level=0;
         !stopArea &&
         level<=this->maxLevel &&
         level<=maxLevel &&
         !cellRefs.empty();
         level++) {
      nextCellRefs.clear();
      newOffsets.clear();

      for (const auto& cellRef : cellRefs) {
        IndexCell  cellIndexData;
        FileOffset cellDataOffset;

        if (!GetIndexCell(level,
                          cellRef.offset,
                          cellIndexData,
                          cellDataOffset)) {
          log.Error() << "Cannot find offset " << cellRef.offset << " in level " << level << " in file '" << scanner.GetFilename() << "'";
          return false;
        }

        size_t spaceLeft=maxCount-offsets.size();

        // Now read the area offsets by type in this index entry

        if (!ReadCellData(*typeConfig,
                          types,
                          cellDataOffset,
                          spaceLeft,
                          level,
                          maxLevel,
                          newOffsets,
                          stopArea)) {
          log.Error() << "Cannot read index data for level " << level << " at offset " << cellDataOffset << " in file '" << scanner.GetFilename() << "'";
        }

        if (stopArea) {
          break;
        }

        if (level<this->maxLevel) {
          size_t cx=cellRef.x*2;
          size_t cy=cellRef.y*2;

          PushCellsForNextLevel(minlon,
                                minlat,
                                maxlon,
                                maxlat,
                                cellIndexData,
                                cellDimension[level+1],
                                cx,cy,
                                nextCellRefs);
        }
      }

      if (!stopArea) {
        offsets.insert(offsets.end(),newOffsets.begin(),newOffsets.end());
      }

      std::swap(cellRefs,nextCellRefs);
    }

    return true;
  }

  void AreaAreaIndex::DumpStatistics()
  {
    indexCache.DumpStatistics(filepart.c_str(),IndexCacheValueSizer());
  }
}
