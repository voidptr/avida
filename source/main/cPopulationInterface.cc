/*
 *  cPopulationInterface.cc
 *  Avida
 *
 *  Created by David on 12/5/05.
 *  Copyright 2005-2006 Michigan State University. All rights reserved.
 *  Copyright 1993-2003 California Institute of Technology.
 *
 */

#include "cPopulationInterface.h"

#include "cGenotype.h"
#include "cHardwareManager.h"
#include "cOrganism.h"
#include "cOrgMessage.h"
#include "cOrgSinkMessage.h"
#include "cPopulation.h"
#include "cPopulationCell.h"
#include "cStats.h"
#include "cTestCPU.h"

#include <assert.h>

#ifndef NULL
#define NULL 0
#endif


bool cPopulationInterface::Divide(cAvidaContext& ctx, cOrganism* parent, cGenome& child_genome)
{
  assert(parent != NULL);
  assert(m_world->GetPopulation().GetCell(cell_id).GetOrganism() == parent);
  return m_world->GetPopulation().ActivateOffspring(ctx, child_genome, *parent);
}

cOrganism * cPopulationInterface::GetNeighbor()
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());
  
  return cell.ConnectionList().GetFirst()->GetOrganism();
}

int cPopulationInterface::GetNumNeighbors()
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());
  
  return cell.ConnectionList().GetSize();
}

void cPopulationInterface::Rotate(int direction)
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());

  if (direction >= 0) cell.ConnectionList().CircNext();
  else cell.ConnectionList().CircPrev();
}

double cPopulationInterface::TestFitness()
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());
  
  return cell.GetOrganism()->GetGenotype()->GetTestFitness();
}

int cPopulationInterface::GetInput()
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());
  return cell.GetInput();
}

int cPopulationInterface::GetInputAt(int& input_pointer)
{
  cPopulationCell& cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());
  return cell.GetInputAt(input_pointer);
}

int cPopulationInterface::Debug()
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());
  return cell.GetOrganism()->GetGenotype()->GetID();
}

const tArray<double> & cPopulationInterface::GetResources()
{
  return m_world->GetPopulation().GetCellResources(cell_id);
}

void cPopulationInterface::UpdateResources(const tArray<double> & res_change)
{
  return m_world->GetPopulation().UpdateCellResources(res_change, cell_id);
}

void cPopulationInterface::Die()
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
  m_world->GetPopulation().KillOrganism(cell);
}

void cPopulationInterface::Kaboom()
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
	m_world->GetPopulation().Kaboom(cell);
}

bool cPopulationInterface::SendMessage(cOrgMessage & mess)
{
  mess.SetSenderID(cell_id);
  mess.SetTime(m_world->GetStats().GetUpdate());
  cPopulationCell& cell = m_world->GetPopulation().GetCell(cell_id);
  if(cell.ConnectionList().GetFirst() == NULL)
    return false;
  mess.SetRecipientID(cell.ConnectionList().GetFirst()->GetID());
  return cell.ConnectionList().GetFirst()->GetOrganism()->ReceiveMessage(mess);
}

cOrgSinkMessage* cPopulationInterface::NetReceive()
{
  cPopulationCell& cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());
  
  const int num_neighbors = cell.ConnectionList().GetSize();
  for (int i = 0; i < num_neighbors; i++) {
    cell.ConnectionList().CircNext();
    
    cOrganism* cur_neighbor = cell.ConnectionList().GetFirst()->GetOrganism();
    cOrgSinkMessage* msg = NULL;
    if (cur_neighbor != NULL && (msg = cur_neighbor->NetPop()) != NULL ) return msg;
  }
  
  return NULL;
}

bool cPopulationInterface::NetRemoteValidate(cAvidaContext& ctx, cOrgSinkMessage* msg)
{
  cOrganism* org = m_world->GetPopulation().GetCell(msg->GetSourceID()).GetOrganism();
  return (org != NULL && org->NetRemoteValidate(ctx, msg->GetOriginalValue()));
}

int cPopulationInterface::ReceiveValue()
{
  cPopulationCell & cell = m_world->GetPopulation().GetCell(cell_id);
  assert(cell.IsOccupied());
  
  const int num_neighbors = cell.ConnectionList().GetSize();
  for (int i = 0; i < num_neighbors; i++) {
    cell.ConnectionList().CircNext();
    
    cOrganism* cur_neighbor = cell.ConnectionList().GetFirst()->GetOrganism();
    if (cur_neighbor == NULL || cur_neighbor->GetSentActive() == false) {
      continue;
    }
    
    return cur_neighbor->RetrieveSentValue();
  }
  
  return 0;
}

bool cPopulationInterface::InjectParasite(cOrganism * parent, const cGenome & injected_code)
{
  assert(parent != NULL);
  assert(m_world->GetPopulation().GetCell(cell_id).GetOrganism() == parent);
  
  return m_world->GetPopulation().ActivateInject(*parent, injected_code);
}

bool cPopulationInterface::UpdateMerit(double new_merit)
{
  return m_world->GetPopulation().UpdateMerit(cell_id, new_merit);
}

