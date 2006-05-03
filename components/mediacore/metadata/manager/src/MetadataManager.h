/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright� 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the �GPL�).
// 
// Software distributed under the License is distributed 
// on an �AS IS� basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
*/

/**
* \file MetadataManager.h
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include "sbIMetadataManager.h"
#include "sbIMetadataHandler.h"
#include <nsCOMPtr.h>

#include <set>

// DEFINES ====================================================================
#define SONGBIRD_METADATAMANAGER_CONTRACTID  "@songbird.org/Songbird/MetadataManager;1"
#define SONGBIRD_METADATAMANAGER_CLASSNAME   "Songbird Metadata Manager Interface"

// {3EBC067F-E52D-4041-A5E4-5C217D4A5FBE}
#define SONGBIRD_METADATAMANAGER_CID { 0x3ebc067f, 0xe52d, 0x4041, { 0xa5, 0xe4, 0x5c, 0x21, 0x7d, 0x4a, 0x5f, 0xbe } }

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
class sbMetadataManager : public sbIMetadataManager
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAMANAGER

  sbMetadataManager();
  virtual ~sbMetadataManager();

  struct sbMetadataHandlerItem
  {
    nsCOMPtr<sbIMetadataHandler> m_Handler;
    PRInt32 m_Vote;
    bool operator < ( const sbMetadataManager::sbMetadataHandlerItem &T ) const
    {
      return m_Vote < T.m_Vote;
    }
    bool operator == ( const sbMetadataManager::sbMetadataHandlerItem &T ) const
    {
      return m_Vote == T.m_Vote;
    }
  };
  class handlerlist_t : public std::set< sbMetadataHandlerItem > {};

};