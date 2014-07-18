/*
 *  cBirthMatingTypeGlobalHandler.h
 *  Avida
 *
 *  Created by David Bryson on 4/1/09.
 *  Copyright 2009-2010 Michigan State University. All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; version 2
 *  of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef cBirthMatingTypeGlobalHandler_h
#define cBirthMatingTypeGlobalHandler_h

#include "cBirthEntry.h"
#include "cBirthSelectionHandler.h"

class cBirthChamber;
//class BirthEntriesManager;

// class to manage the birth entries
class BirthEntriesManager
{
//  class cBirthMatingTypeGlobalHandler;

public:
  class Iterator;

private:

  cWorld* m_world;
  cBirthChamber* m_bc;

  Apto::Array< Apto::List<cBirthEntry> > m_entries;

  int world_size;
  int zone_ct;
  bool mate_by_zone;
  int zone_size;
  int max_zone_bc_size;


public:
  BirthEntriesManager(cWorld* world, cBirthChamber* bc) : m_world(world), m_bc(bc) {
    world_size = -1;
    zone_ct = -1;
    mate_by_zone = false;
    zone_size = -1;
    max_zone_bc_size = -1;
  } // constructor(s)

  int GetWaitingOffspringCount(int zone);
  int GetTotalWaitingOffspringNumber();
  int GetTotalWaitingOffspringNumber(int mating_type);
  int WhichZone(int parent_cell);
  void Insert(cBirthEntry & entry);

  Iterator Begin(int zone);
  Iterator BeginAll();

private:
  void ReConfirmConfiguration();

public:
  class Iterator
  {
    friend class BirthEntriesManager;
  private:
    BirthEntriesManager* m_bem;
    int zone;
    bool all_zones;
    int current_zone;
    Apto::List<cBirthEntry>::Iterator m_it;
    Iterator(); // @not implemented
    Iterator(BirthEntriesManager* manager, int _zone = 0, bool _all_zones = true)
      : zone(_zone)
      , m_bem(manager)
      , m_it(manager->m_entries[zone].Begin())
      , all_zones(_all_zones)
      , current_zone(_zone)
    {
    }

  public:
    inline cBirthEntry* GetPos(int pos)
    {
      int cur_pos = pos;
      if (all_zones) // figure out where this position fits
      {
        for (int i = 0; i < m_bem->zone_ct; i++)
        {
          if (cur_pos >= m_bem->m_entries[i].GetSize())
          {
            cur_pos -= m_bem->m_entries[i].GetSize();
          }
          else
          {
            m_it = m_bem->m_entries[i].Begin();
            for (int j = 0; j <= cur_pos; j++)
            {
              m_it.Next();
            }
            return m_it.Get();
          }
        }
      }
      else
      {
        m_it = m_bem->m_entries[zone].Begin();
        for (int j = 0; j <= cur_pos; j++)
        {
          m_it.Next();
        }
        return m_it.Get();
      }
      return NULL;
    }

    inline cBirthEntry* Get() {
      return m_it.Get();
    }

    inline cBirthEntry* Next() {
      if (m_it.Next() == NULL) // end of the line
      {
        if (all_zones && (++current_zone) < m_bem->zone_ct ) // ok, we can try rolling over to the next one
        {
          m_it = m_bem->m_entries[current_zone].Begin();
          return m_it.Next();
        }
        else
        {
          return NULL; // all done!
        }
      }
      else
      {
        return m_it.Get();
      }
    }
  };
};

class cBirthMatingTypeGlobalHandler : public cBirthSelectionHandler
{
  friend class BirthEntriesManager;

private:
  cWorld* m_world;
  cBirthChamber* m_bc;
  BirthEntriesManager m_bem;

  int getTaskID(cString task_name, cWorld* world);
  void storeOffspring(cAvidaContext& ctx, const Genome& offspring, cOrganism* parent);
  cBirthEntry* selectMate(cAvidaContext& ctx, const Genome& offspring, cOrganism* parent, int which_mating_type, int mate_choice_method);
  //int getWaitingOffspringMostTask(int which_mating_type, int task_id);
  bool compareBirthEntries(cAvidaContext& ctx, cOrganism* parent, int mate_choice_method, const cBirthEntry& entry1, const cBirthEntry& entry2);


public:
  cBirthMatingTypeGlobalHandler(cWorld* world, cBirthChamber* bc) : m_world(world), m_bc(bc), m_bem(world, bc) {
    //m_bem = new BirthEntriesManager(this);
    ;
  }
  ~cBirthMatingTypeGlobalHandler();
  
  cBirthEntry* SelectOffspring(cAvidaContext& ctx, const Genome& offspring, cOrganism* parent);
  
  int GetWaitingOffspringNumber(int which_mating_type);
  void PrintBirthChamber(const cString& filename);
  //void GetWaitingOffspringTaskData(int task_id, float results_array[]); //CHC Note: Overridden functionality still needs to be implemented
};



#endif
