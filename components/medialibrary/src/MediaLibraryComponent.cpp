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
* \file  MediaLibraryComponent.cpp
* \brief Songbird MediaLibrary Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"

#include "DatabaseQuery.h"
#include "DatabaseResult.h"
#include "DatabaseEngine.h"

#include "MediaScan.h"

#define NS_GENERIC_FACTORY_SIMPLETON_CONSTRUCTOR( _Interface )                  \
  static _Interface * _Interface##SimpletonConstructor( void )                  \
  {                                                                             \
  static _Interface * m_Simpleton = nsnull;                                     \
  NS_IF_ADDREF( m_Simpleton ? m_Simpleton : ( NS_IF_ADDREF( m_Simpleton = new _Interface() ), m_Simpleton ) ); \
  return m_Simpleton;                                                         \
  }                                                                             \
  NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR( _Interface, _Interface##SimpletonConstructor )

NS_GENERIC_FACTORY_SIMPLETON_CONSTRUCTOR(CDatabaseEngine)

NS_GENERIC_FACTORY_CONSTRUCTOR(CDatabaseQuery)
NS_GENERIC_FACTORY_CONSTRUCTOR(CDatabaseResult)
NS_GENERIC_FACTORY_CONSTRUCTOR(CMediaScan)
NS_GENERIC_FACTORY_CONSTRUCTOR(CMediaScanQuery)

static nsModuleComponentInfo sbMediaLibrary[] =
{
  {
    SONGBIRD_DATABASEENGINE_CLASSNAME,
    SONGBIRD_DATABASEENGINE_CID,
    SONGBIRD_DATABASEENGINE_CONTRACTID,
    CDatabaseEngineConstructor
  },

  {
    SONGBIRD_DATABASEQUERY_CLASSNAME,
    SONGBIRD_DATABASEQUERY_CID,
    SONGBIRD_DATABASEQUERY_CONTRACTID,
    CDatabaseQueryConstructor
  },

  {
    SONGBIRD_DATABASERESULT_CLASSNAME,
    SONGBIRD_DATABASERESULT_CID,
    SONGBIRD_DATABASERESULT_CONTRACTID,
    CDatabaseResultConstructor
  },

  {
    SONGBIRD_MEDIASCAN_CLASSNAME,
    SONGBIRD_MEDIASCAN_CID,
    SONGBIRD_MEDIASCAN_CONTRACTID,
    CMediaScanConstructor
  },

  {
    SONGBIRD_MEDIASCANQUERY_CLASSNAME,
    SONGBIRD_MEDIASCANQUERY_CID,
    SONGBIRD_MEDIASCANQUERY_CONTRACTID,
    CMediaScanQueryConstructor
  },
  
};

NS_IMPL_NSGETMODULE("SongbirdMediaLibraryComponent", sbMediaLibrary)
