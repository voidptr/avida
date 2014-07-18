/*
 *  cBirthMatingTypeGlobalHandler.cc
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

#include "cBirthMatingTypeGlobalHandler.h"

#include "avida/core/Genome.h"
#include "avida/output/File.h"

#include "cBirthChamber.h"
#include "cOrganism.h"
#include "cWorld.h"
#include "cEnvironment.h"
#include "cTaskEntry.h"
#include "cString.h"

#include <iostream>


cBirthMatingTypeGlobalHandler::~cBirthMatingTypeGlobalHandler()
{
}

cBirthEntry* cBirthMatingTypeGlobalHandler::SelectOffspring(cAvidaContext& ctx, const Genome& offspring, cOrganism* parent)
{
  int parent_sex = parent->GetPhenotype().GetMatingType();
  
  //Parent has not sexually matured
  if (parent_sex == MATING_TYPE_JUVENILE) {
    return NULL;
  }
  
  //Parent was a female
  if (parent_sex == MATING_TYPE_FEMALE) {
    int mate_choice_method = parent->GetPhenotype().GetMatePreference();
    return selectMate(ctx, offspring, parent, MATING_TYPE_MALE, mate_choice_method);
  }
  
  //Parent was a male
  if (parent_sex == MATING_TYPE_MALE) {
    if (m_world->GetConfig().LEKKING.Get() != 0) {
      //Lekking is turned on, so if the parent is a male, force the offspring into the birth chamber,
      //  and wait for a female
      storeOffspring(ctx, offspring, parent);
      return NULL;
    } else {
      //Lekking is off, so find a mate
      //if there is none, store the current one
      //if there is, mate with it
      int mate_choice_method = parent->GetPhenotype().GetMatePreference();
      return selectMate(ctx, offspring, parent, MATING_TYPE_FEMALE, mate_choice_method);
    }
  }
  
  //One of the previous conditions should have been met, so hopefully the following line will never
  //be executed, but just in case...
  return NULL;
}


//Returns the number of offspring of a specific mating type waiting in the birth chamber
int cBirthMatingTypeGlobalHandler::GetWaitingOffspringNumber(int which_mating_type)
{
  return m_bem.GetTotalWaitingOffspringNumber(which_mating_type);

}
//Returns the id number that corresponds to a particular task name
//Returns -1 if the task is not being monitored in the current environment
int cBirthMatingTypeGlobalHandler::getTaskID(cString task_name, cWorld* world)
{
  int num_tasks = world->GetEnvironment().GetNumTasks();
  for (int i = 0; i < num_tasks; i++) {
    if (world->GetEnvironment().GetTask(i).GetName() == task_name) return i;
  }
  return -1;
}

//Stores the specified offspring in the specified birth chamber
void cBirthMatingTypeGlobalHandler::storeOffspring(cAvidaContext&, const Genome& offspring, cOrganism* parent)
{
  //First, don't bother doing ANYTHING if LEKKING is turned on and the parent is a female --
  // -- in this case, there's no point in putting her offspring in the birth chamber because 
  // the males will never choose it; the males always go directly into the birth chamber
  if ((m_world->GetConfig().LEKKING.Get() != 0) & (parent->GetPhenotype().GetMatingType() == MATING_TYPE_FEMALE)) {
    return;
  }


  cBirthEntry * tmp = new cBirthEntry();
  m_bc->StoreAsEntry(offspring, parent, *tmp);
  m_bem.Insert(*tmp);

  /*
  cBirthEntry tmp;
  m_bc->StoreAsEntry(offspring, parent, tmp);
  m_bem.Insert(tmp);
*/

  /*
  //Find an empty entry
  //If there are none, make room for one
  //But if the birth chamber is at the size limit already, over-write the oldest one
  int oldest_index = -1;
  int entry_list_size = m_entries.GetSize();
  //Loop through all the birth entries
  for (int i = 0; i < entry_list_size; i++) {
    if (m_bc->ValidateBirthEntry(m_entries[i])) {
      //Current entry is valid, so let's just keep track of the oldest one
      if (oldest_index == -1) oldest_index = i;
      else oldest_index = (m_entries[i].timestamp < m_entries[oldest_index].timestamp ? i : oldest_index);
    } else {
      //Current entry is empty, so let's use this one
      m_bc->ClearEntry(m_entries[i]);
      m_bc->StoreAsEntry(offspring, parent, m_entries[i]);
      return;
    }
  }

  //If we're still here, it means we didn't find any empty entries
  //So, let's make room for one and then store it; but if the list is already at its max size,
  // we'll just have to over-write the oldest one
  int store_index = m_entries.GetSize();
  int max_buffer_size = m_world->GetConfig().MAX_GLOBAL_BIRTH_CHAMBER_SIZE.Get();
  if (store_index >= max_buffer_size) {
    //m_bc->ClearEntry(m_entries[oldest_index]);
    store_index = oldest_index;
  } else {
    m_entries.Resize(store_index + 1);
  }
  
  m_bc->ClearEntry(m_entries[store_index]);
  m_bc->StoreAsEntry(offspring, parent, m_entries[store_index]);
  */
}

//Compares two birth entries and decides which one is preferred
//Returns true if the first one is "better"
//Returns false if the second one is
bool cBirthMatingTypeGlobalHandler::compareBirthEntries(cAvidaContext& ctx,
                                                        cOrganism* parent,
                                                        int mate_choice_method,
                                                        const cBirthEntry& entry1,
                                                        const cBirthEntry& entry2)
{
  assert(mate_choice_method != MATE_PREFERENCE_RANDOM);
  
  double cv = m_world->GetConfig().MATE_ASSESSMENT_CV.Get();
  double value1 = 0;
  double value2 = 0;
  double target = 0;
  
  switch (mate_choice_method) {
    case MATE_PREFERENCE_HIGHEST_DISPLAY_A: //Prefers to mate with the organism with the highest value of mating display A
      value1 = (double) entry1.GetMatingDisplayA();
      value2 = (double) entry2.GetMatingDisplayA();
      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (value1 > value2);
      break;
    
    case MATE_PREFERENCE_HIGHEST_DISPLAY_B: //Prefers to mate with the organism with the highest value of mating display B
      value1 = (double) entry1.GetMatingDisplayB();
      value2 = (double) entry2.GetMatingDisplayB();
      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (value1 > value2);
      break;
      
    case MATE_PREFERENCE_HIGHEST_MERIT:
      value1 = (double) entry1.merit.GetDouble();
      value2 = (double) entry2.merit.GetDouble();
      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (value1 > value2);
      break;
    case MATE_PREFERENCE_LOWEST_DISPLAY_A: //Prefers to mate with the organism with the highest value of mating display A
      value1 = (double) entry1.GetMatingDisplayA();
      value2 = (double) entry2.GetMatingDisplayA();
      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (value1 < value2);
      break;
    
    case MATE_PREFERENCE_LOWEST_DISPLAY_B: //Prefers to mate with the organism with the highest value of mating display B
      value1 = (double) entry1.GetMatingDisplayB();
      value2 = (double) entry2.GetMatingDisplayB();
      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (value1 < value2);
      break;
      
    case MATE_PREFERENCE_LOWEST_MERIT:
      value1 = (double) entry1.merit.GetDouble();
      value2 = (double) entry2.merit.GetDouble();
      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (value1 < value2);
      break;

    case MATE_PREFERENCE_TARGET_DISPLAY_A: //Prefers to mate with the organism with the closest value of mating display A
      value1 = (double) entry1.GetMatingDisplayA();
      value2 = (double) entry2.GetMatingDisplayA();

      target = (double) parent->GetPhenotype().GetCurMatingDisplayA();

      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (std::abs(target - value1) < std::abs(target - value2));
      break;
    
    case MATE_PREFERENCE_TARGET_DISPLAY_B: //Prefers to mate with the organism with the highest value of mating display B
      value1 = (double) entry1.GetMatingDisplayB();
      value2 = (double) entry2.GetMatingDisplayB();

      target = (double) parent->GetPhenotype().GetCurMatingDisplayB();

      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (std::abs(target - value1) < std::abs(target - value2));
      break;

    case MATE_PREFERENCE_TARGET_DISPLAY_C: //Prefers to mate with the organism with the highest value of mating display C
      value1 = (char) entry1.GetMatingDisplayC();
      value2 = (char) entry2.GetMatingDisplayC();

      target = (char) parent->GetPhenotype().GetCurMatingDisplayC();

      if (m_world->GetConfig().NOISY_MATE_ASSESSMENT.Get()) {
        value1 += ctx.GetRandom().GetRandNormal(0, value1*cv);
        value2 += ctx.GetRandom().GetRandNormal(0, value2*cv);
      }
      return (std::abs(target - value1) < std::abs(target - value2));
      break;
  }
  
  //If we're still here... just decide randomly since we need to give some return value...
  return ctx.GetRandom().P(0.5);
}

//Selects a mate for the current offspring/gamete
//If none is found, it returns NULL
cBirthEntry* cBirthMatingTypeGlobalHandler::selectMate(cAvidaContext& ctx,
                                                       const Genome& offspring,
                                                       cOrganism* parent,
                                                       int which_mating_type,
                                                       int mate_choice_method)
{



  //Loop through the entry list and find a mate
  //If none are found, store the current offspring and return NULL
  int num_waiting = m_bem.GetWaitingOffspringCount(m_bem.WhichZone(parent->GetCellID()));
  if (m_bem.WhichZone(parent->GetCellID()) == 0 && parent->GetPhenotype().GetMatingType() == MATING_TYPE_FEMALE)
  {
    cout << "  Selecting Mate Zone 0 - ";
    //cout << parent->GetPhenotype().GetMatingType();
    cout << " (female) ";
    cout << parent->GetPhenotype().CalcCurrentMerit();
    cout << " - ";
    cout << num_waiting;
    //cout << "\n";
  }

  if (m_bem.WhichZone(parent->GetCellID()) == 1 && parent->GetPhenotype().GetMatingType() == MATING_TYPE_FEMALE)
  {
    cout << "                                                                                        Selecting Mate Zone 1 - ";
    //cout << parent->GetPhenotype().GetMatingType();
    cout << " (female) ";
    cout << parent->GetPhenotype().CalcCurrentMerit();
    cout << " - ";
    cout << num_waiting;
    //cout << "\n";
  }
  if (num_waiting == 0) {
    storeOffspring(ctx, offspring, parent);
    return NULL;
  }
  
  int selected_index = -1;
  
  if (m_world->GetConfig().FORCED_MATE_PREFERENCE.Get() != -1) {
    mate_choice_method = m_world->GetConfig().FORCED_MATE_PREFERENCE.Get();
  }
  
  if (mate_choice_method == MATE_PREFERENCE_RANDOM) {
    //This is a non-choosy individual, so pick a mate randomly!
    //First, get a list of every element of m_entries that contains a waiting offspring (of the compatible sex)
    //Then pick one at random
    
    Apto::Array<int> compatible_entries; //This will hold a list of all the compatible birth entries waiting in the birth chamber
    compatible_entries.Resize(num_waiting, -1);
    int last_compatible = -1; //The index of the last entry in compatible_entries holding a compatible m_entries index

    BirthEntriesManager::Iterator it = m_bem.Begin(m_bem.WhichZone(parent->GetCellID()));
    int i = 0;
    while (it.Next() != NULL)
    {
      if (m_bc->ValidateBirthEntry(*it.Get())) { //Is the current entry valid/alive?

        //Here, check to see if the current entry belongs to the parent's group! @CHC
        bool groups_match = false;
        if (!(m_world->GetConfig().MATE_IN_GROUPS.Get())) {
          groups_match = true; //Within-group mating is turned off, so don't need to check
        } else {
          //Within-group mating is turned on, so we need to check if the parent's group is the same
          if (parent->HasOpinion()) {
            groups_match = (it.Get()->GetGroupID() == parent->GetOpinion().first);
          }
        }

        //Here, check to see if the current entry shares a mate select mate_ID @RCK
        bool mate_id_matches = false;
        if (!(m_world->GetConfig().ALLOW_MATE_SELECTION.Get())) {
          mate_id_matches = true; //mate_select mating is turned off, so don't need to check
        } else {
          int mate_id = parent->GetPhenotype().MateSelectID();
          if (mate_id > 0) { // parent has a mate ID that we care about.
            mate_id_matches = (it.Get()->GetMateID() == mate_id);
          }
        }

        if (groups_match && mate_id_matches) {
          if (it.Get()->GetMatingType() == which_mating_type) { //Is the current entry a compatible mating type?
            last_compatible++;
            compatible_entries[last_compatible] = i;
          }
        }
      }
      i++;
    }
    if (last_compatible > -1) { //Don't bother picking one if we haven't found any compatible entries
      selected_index = compatible_entries[ctx.GetRandom().GetUInt(last_compatible+1)];
    }
    if (parent->GetPhenotype().GetMatingType() == MATING_TYPE_FEMALE)
    {
      cout << " compatible ct (random) " << last_compatible << "\n";
    }
//    cout << "last compatible " << last_compatible << "\n";

  } else {
    //This is a choosy female, so go through all the mates and pick the "best" one!
    BirthEntriesManager::Iterator it = m_bem.Begin(m_bem.WhichZone(parent->GetCellID()));
    cBirthEntry * latest_selected = NULL;
    int i = 0;
    int compatible_ct = 0;
    while (it.Next() != NULL)
    {
      if (m_bc->ValidateBirthEntry(*it.Get())) { //Is the current entry valid/alive?

        //Here, check to see if the current entry belongs to the parent's group! @CHC
        bool groups_match = false;
        if (!(m_world->GetConfig().MATE_IN_GROUPS.Get())) {
          groups_match = true; //Within-group mating is turned off, so don't need to check
        } else {
          //Within-group mating is turned on, so we need to check if the parent's group is the same
          if (parent->HasOpinion()) {
            groups_match = (it.Get()->GetGroupID() == parent->GetOpinion().first);
          }
        }

        //Here, check to see if the current entry shares a mate select mate_ID @RCK
        bool mate_id_matches = false;
        if (!(m_world->GetConfig().ALLOW_MATE_SELECTION.Get())) {
          mate_id_matches = true; //mate_select mating is turned off, so don't need to check
        } else {
          int mate_id = parent->GetPhenotype().MateSelectID();
          if (mate_id > 0) { // parent has a mate ID that we care about.
            mate_id_matches = (it.Get()->GetMateID() == mate_id);
          }
        }

        if (groups_match && mate_id_matches) {
          if (it.Get()->GetMatingType() == which_mating_type) { //Is the current entry a compatible mating type?
            compatible_ct++;
            if (selected_index == -1)
            {
              selected_index = i;
              latest_selected = it.Get();
            }
            else if (compareBirthEntries(ctx, parent, mate_choice_method, *it.Get(), *latest_selected))
            {
              selected_index = i;
              latest_selected = it.Get();
            }
          }
        }
      }
      i++;
    }
    if (parent->GetPhenotype().GetMatingType() == MATING_TYPE_FEMALE)
    {
      cout << " compatible ct (choosy) " << compatible_ct << "\n";
    }
  }
  
  if (selected_index == -1) {
    //None found: Store the current one and return NULL
    storeOffspring(ctx, offspring, parent);
    return NULL;
  }
  //cout << "Selected " << m_entries[selected_index].GetPhenotypeString() << "\n";
  //return &(m_entries[selected_index]);
  BirthEntriesManager::Iterator it = m_bem.Begin(m_bem.WhichZone(parent->GetCellID()));
  return it.GetPos(selected_index);
  
}


/*
int cBirthMatingTypeGlobalHandler::getWaitingOffspringMostTask(int which_mating_type, int task_id)
{
  int selected_index = -1;
  int num_waiting = m_entries.GetSize();
  if (num_waiting == 0) return -1;

  for (int i = 0; i < num_waiting; i++) {
    if (m_bc->ValidateBirthEntry(m_entries[i])) {
      if (m_entries[i].GetMatingType() == which_mating_type) {
        if (selected_index == -1) selected_index = i;
        else selected_index = ((m_entries[i].GetParentTaskCount()[task_id] > m_entries[selected_index].GetParentTaskCount()[task_id]) ? i : selected_index);
      }
    }
  }
  return selected_index;
} */

//Outputs the entire birth chamber to a file
void cBirthMatingTypeGlobalHandler::PrintBirthChamber(const cString& filename)
{
  Apto::String file_path((const char*)filename);
  Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), file_path);
  df->WriteTimeStamp();
  df->WriteComment(cBirthEntry::GetPhenotypeStringFormat());
  df->Endl();
  
  std::ofstream& df_stream = df->OFStream();
  
  BirthEntriesManager::Iterator it = m_bem.BeginAll();
  while(it.Next() != NULL) {
  //for (int i = 0; i < m_entries.GetSize(); i++) {
    if (m_bc->ValidateBirthEntry(*it.Get())) {
      df_stream << it.Get()->GetPhenotypeString() << endl;
    }
  }
}



int BirthEntriesManager::GetTotalWaitingOffspringNumber(int mating_type)
{
  ReConfirmConfiguration();
  int num_waiting = 0;

  for (int i = 0; i < zone_ct; i++)
  {
    Apto::List<cBirthEntry>::Iterator it = m_entries[i].Begin();
    while(it.Next() != NULL) {
      if (m_bc->ValidateBirthEntry(*it.Get())) {
        if (it.Get()->GetMatingType() == mating_type) num_waiting++;
      }
    }
  }
  return num_waiting;
}

int BirthEntriesManager::GetTotalWaitingOffspringNumber()
{
  ReConfirmConfiguration();
  int num_waiting = 0;

  for (int i = 0; i < m_entries.GetSize(); i++)
  {
    num_waiting += m_entries[i].GetSize();
  }
  return num_waiting;
}


int BirthEntriesManager::WhichZone(int parent_cell)
{
  ReConfirmConfiguration();
  int zone = parent_cell / zone_size;
  if (zone >= zone_ct)
    zone = zone_ct - 1;

  return zone;

}

int BirthEntriesManager::GetWaitingOffspringCount(int zone)
{
  ReConfirmConfiguration();
  if (mate_by_zone)
    return m_entries[zone].GetSize();
  else
  {
    return GetTotalWaitingOffspringNumber();
  }
}

void BirthEntriesManager::Insert(cBirthEntry & entry)
{
  ReConfirmConfiguration();
  int target_zone = WhichZone(entry.GetParentCellID());

  m_entries[ target_zone ].Push(entry);
  if (m_entries[target_zone].GetSize() > max_zone_bc_size)
  {
    cBirthEntry tmp = m_entries[target_zone].PopRear();
    m_bc->ClearEntry(tmp);
    //delete &tmp;
  }
    //m_bc->ClearEntry(m_entries[target_zone].PopRear());
}

BirthEntriesManager::Iterator BirthEntriesManager::Begin(int zone)
{
  ReConfirmConfiguration();
  if (mate_by_zone)
    return Iterator(this, zone, false);
  else
    return Iterator(this);
}

BirthEntriesManager::Iterator BirthEntriesManager::BeginAll()
{
  ReConfirmConfiguration();

  return Iterator(this);
}

void BirthEntriesManager::ReConfirmConfiguration()
{
  mate_by_zone = m_world->GetConfig().MATE_BY_BIRTH_ZONE.Get();
  max_zone_bc_size = m_world->GetConfig().MAX_GLOBAL_BIRTH_CHAMBER_SIZE.Get() / m_world->GetConfig().BIRTH_ZONES.Get();
  if (zone_ct != m_world->GetConfig().BIRTH_ZONES.Get()) // well shit. rejigger everything.
  {

    Apto::List<cBirthEntry> tmp;

    // join the lists together into a temporary one.
    for (int i = 1; i < zone_ct; i++)
    {
      tmp += m_entries[i];
    }

    zone_ct = m_world->GetConfig().BIRTH_ZONES.Get();
    m_entries.ResizeClear(zone_ct);
    world_size = (m_world->GetConfig().WORLD_X.Get() * m_world->GetConfig().WORLD_Y.Get());
    zone_size = world_size / zone_ct;

    Apto::List<cBirthEntry>::Iterator it = tmp.Begin();
    int ct = 0;
    while(it.Next() != NULL && ct < max_zone_bc_size)
    {
      int target_zone = it.Get()->GetParentCellID() / zone_size;
      if (target_zone >= zone_ct) { target_zone = zone_ct - 1; }
      m_entries[ target_zone ].PushRear(*it.Get());
      ct++;
    }
  }

}
