/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
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

#ifndef __SB_LOCALDATABASEDIFFINGSERVICE_H__
#define __SB_LOCALDATABASEDIFFINGSERVICE_H__

#include <sbILibraryDiffingService.h>

#include <nsIClassInfo.h>
#include <nsIGenericFactory.h>
#include <nsIThread.h>

#include <sbILibrary.h>
#include <sbILibraryChangeset.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsInterfaceHashtable.h>

#include <sbLibraryChangeset.h>

class sbLocalDatabaseDiffingService : public sbILibraryDiffingService,
                                      public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBILIBRARYDIFFINGSERVICE

  sbLocalDatabaseDiffingService();
  
  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  /** 
   * Initialize the local database diffing service.
   * \throw NS_ERROR_ALREADY_INITIALIZED if called more than once.
   */
  nsresult Init();

  /**
   * Get all property info elements from the property manager.
   */
  nsresult GetPropertyIDs(nsIStringEnumerator **aPropertyIDs);

  /**
   * Create a sbILibraryChange from a source and destination sbIMediaItem.
   *
   * \param aSourceItem The source item.
   * \param aDestinationItem The destination item.
   * \param[out] aLibraryChange The newly created library change.
   */
  nsresult CreateLibraryChangeFromItems(sbIMediaItem *aSourceItem, 
                                        sbIMediaItem *aDestinationItem,
                                        sbILibraryChange **aLibraryChange);
  
  nsresult CreateItemAddedLibraryChange(sbIMediaItem *aSourceItem,
                                        sbILibraryChange **aLibraryChange);

  nsresult CreateItemMovedLibraryChange(sbIMediaItem *aSourceItem,
                                        PRUint32 aItemOrdinal,
                                        sbILibraryChange **aLibraryChange);

  nsresult CreateItemDeletedLibraryChange(sbIMediaItem *aDestinationItem,
                                          sbILibraryChange **aLibraryChange);

  /**
   * Create an array of sbIPropertyChange elements from a source
   * and destination sbIPropertyArray.
   * 
   * \param aSourceProperties The source property array.
   * \param aDestinationProperties The destination property array.
   * \param[out] aPropertyChanges The newly created property changes array.
   */
  nsresult CreatePropertyChangesFromProperties(sbIPropertyArray *aSourceProperties,
                                               sbIPropertyArray *aDestinationProperties,
                                               nsIArray **aPropertyChanges);

  /**
   * Create a sbILibraryChangeset from a source
   * and destination sbIMediaList.
   *
   * \param aSourceList The source list.
   * \param aDestinationList The destination list.
   * \param[out] 
   */
  nsresult CreateLibraryChangesetFromLists(sbIMediaList *aSourceList,
                                           sbIMediaList *aDestinationList,
                                           sbILibraryChangeset **aLibraryChangeset);
  
  /** 
   * Create an sbILibraryChangeset from a source and a destination
   * sbILibrary.
   *
   * \param aSourceLibrary The source library.
   * \param aDestinationLibrary The destination library.
   * \param[out] aLibraryChangeset The newly created library changeset.
   */
  nsresult CreateLibraryChangesetFromLibraries(sbILibrary *aSourceLibrary,
                                               sbILibrary *aDestinationLibrary,
                                               sbILibraryChangeset **aLibraryChangeset);

  nsresult CreateLibraryChangesetFromListsToLibrary(nsIArray *aSourceLists,
                                                    sbILibrary *aDestinationLibrary,
                                                    sbILibraryChangeset **aLibraryChangeset);

private:
  ~sbLocalDatabaseDiffingService();

protected:
  nsCOMPtr<nsIThread> mProcessorThread;
  nsCOMPtr<sbIPropertyManager> mPropertyManager;

//  XXXAus: These are placeholders for future implementation.
//  nsInterfaceHashtableMT<> mObservers;
//  nsInterfaceHashtableMT<> mChangesets;
};

class sbLocalDatabaseDiffingServiceEnumerator : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbLocalDatabaseDiffingServiceEnumerator();

  nsresult Init();

  nsresult GetArray(nsIArray **aArray);

protected:
  ~sbLocalDatabaseDiffingServiceEnumerator();

private:
  nsCOMPtr<nsIMutableArray> mArray;

};

#endif /* __SB_LOCALDATABASEDIFFINGSERVICE_H__ */
