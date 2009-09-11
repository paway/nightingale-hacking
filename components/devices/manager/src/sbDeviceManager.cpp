/* vim: set sw=2 :miv */
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

#include "sbDeviceManager.h"

#include <nsIAppStartupNotifier.h>
#include <nsIClassInfoImpl.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
#include <nsIProgrammingLanguage.h>
#include <nsISupportsPrimitives.h>

#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsIDOMWindow.h>

#include "sbIDeviceController.h"
#include "sbDeviceEvent.h"
#include "sbDeviceUtils.h"

#include <sbIPrompter.h>
#include <sbILibraryManager.h>

/* observer topics */
#define NS_PROFILE_STARTUP_OBSERVER_ID          "profile-after-change"
#define NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID "quit-application-granted"
#define NS_PROFILE_SHUTDOWN_OBSERVER_ID         "profile-before-change"
#define SB_MAINWIN_PRESENTED_OBSERVER_ID        "songbird-main-window-presented"

NS_IMPL_THREADSAFE_ADDREF(sbDeviceManager)
NS_IMPL_THREADSAFE_RELEASE(sbDeviceManager)
NS_IMPL_QUERY_INTERFACE7_CI(sbDeviceManager,
                            sbIDeviceManager2,
                            sbIDeviceControllerRegistrar,
                            sbIDeviceRegistrar,
                            sbIDeviceEventTarget,
                            nsISupportsWeakReference,
                            nsIClassInfo,
                            nsIObserver)
NS_IMPL_CI_INTERFACE_GETTER5(sbDeviceManager,
                             sbIDeviceManager2,
                             sbIDeviceControllerRegistrar,
                             sbIDeviceRegistrar,
                             sbIDeviceEventTarget,
                             nsISupportsWeakReference)

NS_DECL_CLASSINFO(sbDeviceManager)
NS_IMPL_THREADSAFE_CI(sbDeviceManager)

sbDeviceManager::sbDeviceManager()
 : mMonitor(nsnull)
{
}

sbDeviceManager::~sbDeviceManager()
{
  /* destructor code */
}

template<class T>
PLDHashOperator sbDeviceManager::EnumerateIntoArray(const nsID& aKey,
                                                    T* aData,
                                                    void* aArray)
{
  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/* readonly attribute nsIArray sbIDeviceManager::marshalls; */
NS_IMETHODIMP sbDeviceManager::GetMarshalls(nsIArray * *aMarshalls)
{
  NS_ENSURE_ARG_POINTER(aMarshalls);

  nsresult rv;

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count;
  count = mMarshalls.EnumerateRead(sbDeviceManager::EnumerateIntoArray,
                                   array.get());

  // we can't trust the count returned from EnumerateRead because that won't
  // tell us about erroring on the last element
  rv = array->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count < mMarshalls.Count()) {
    return NS_ERROR_FAILURE;
  }

  return CallQueryInterface(array, aMarshalls);
}

/* sbIDeviceMarshall sbIDeviceManager::getMarshallByID (in nsIDPtr aIDPtr); */
NS_IMETHODIMP sbDeviceManager::GetMarshallByID(const nsID * aIDPtr,
                                               sbIDeviceMarshall **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_POINTER(aIDPtr);

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    nsresult rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool succeded = mMarshalls.Get(*aIDPtr, _retval);
  return succeded ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* void sbIDeviceManager::updateDevices (); */
NS_IMETHODIMP sbDeviceManager::UpdateDevices()
{
  nsCOMPtr<nsIArray> controllers;
  nsresult rv = this->GetControllers(getter_AddRefs(controllers));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = controllers->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<sbIDeviceController> controller;
    rv = controllers->QueryElementAt(i, NS_GET_IID(sbIDeviceController),
                                     getter_AddRefs(controller));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = controller->ConnectDevices();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/* sbIDeviceEvent createEvent (in unsigned long aType,
                               [optional] in nsIVariant aData,
                               [optional] in nsISupports aOrigin); */
NS_IMETHODIMP sbDeviceManager::CreateEvent(PRUint32 aType,
                                           nsIVariant *aData,
                                           nsISupports *aOrigin,
                                           sbIDeviceEvent **_retval)
{
  return sbDeviceEvent::CreateEvent(aType, aData, aOrigin, _retval);
}

/* sbIDevice sbIDeviceManager::getDeviceForItem(in sbIMediaItem aItem); */
NS_IMETHODIMP sbDeviceManager::GetDeviceForItem(sbIMediaItem* aMediaItem,
                                                sbIDevice**   _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // get the list of devices
  nsCOMPtr<nsIArray> deviceList;
  rv = GetDevices(getter_AddRefs(deviceList));
  NS_ENSURE_SUCCESS(rv, rv);

  // check each device for item
  PRUint32 deviceCount;
  rv = deviceList->GetLength(&deviceCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < deviceCount; i++) {
    // get the next device
    nsCOMPtr<sbIDevice> device = do_QueryElementAt(deviceList, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // try getting the device library for the item.  if one is found, the item
    // belongs to the device
    nsCOMPtr<sbIDeviceLibrary> library;
    rv = sbDeviceUtils::GetDeviceLibraryForItem(device,
                                                aMediaItem,
                                                getter_AddRefs(library));
    if (NS_SUCCEEDED(rv)) {
      device.swap(*_retval);
      return NS_OK;
    }
  }

  *_retval = nsnull;

  return NS_OK;
}

/* readonly attribute nsIArray sbIDeviceControllerRegistrar::controllers; */
NS_IMETHODIMP sbDeviceManager::GetControllers(nsIArray * *aControllers)
{
  NS_ENSURE_ARG_POINTER(aControllers);

  nsresult rv;

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIMutableArray> array = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count;
  count = mControllers.EnumerateRead(sbDeviceManager::EnumerateIntoArray,
                                     array.get());

  // we can't trust the count returned from EnumerateRead because that won't
  // tell us about erroring on the last element
  rv = array->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count < mControllers.Count()) {
    return NS_ERROR_FAILURE;
  }

  return CallQueryInterface(array, aControllers);
}

/* void sbIDeviceControllerRegistrar::registerController (
        in sbIDeviceController aController); */
NS_IMETHODIMP sbDeviceManager::RegisterController(sbIDeviceController *aController)
{
  NS_ENSURE_ARG_POINTER(aController);

  nsresult rv;

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsID* id;
  rv = aController->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(id);

  PRBool succeeded = mControllers.Put(*id, aController);
  NS_Free(id);
  return succeeded ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* void sbIDeviceControllerRegistrar::unregisterController (in sbIDeviceController aController); */
NS_IMETHODIMP sbDeviceManager::UnregisterController(sbIDeviceController *aController)
{
  NS_ENSURE_ARG_POINTER(aController);

  nsresult rv;

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsID* id;
  rv = aController->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(id);

  mControllers.Remove(*id);
  NS_Free(id);
  return NS_OK;
}

/* sbIDeviceController sbIDeviceControllerRegistrar::getController (in nsIDPtr aControllerId); */
NS_IMETHODIMP sbDeviceManager::GetController(const nsID * aControllerId,
                                             sbIDeviceController **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_POINTER(aControllerId);

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    nsresult rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }


  PRBool succeded = mControllers.Get(*aControllerId, _retval);
  return succeded ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* readonly attribute nsIArray sbIDeviceRegistrar::devices; */
NS_IMETHODIMP sbDeviceManager::GetDevices(nsIArray * *aDevices)
{
  NS_ENSURE_ARG_POINTER(aDevices);

  nsresult rv;

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 count;
  count = mDevices.EnumerateRead(sbDeviceManager::EnumerateIntoArray,
                                 array.get());

  // we can't trust the count returned from EnumerateRead because that won't
  // tell us about erroring on the last element
  rv = array->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count < mDevices.Count()) {
    return NS_ERROR_FAILURE;
  }

  return CallQueryInterface(array, aDevices);
}

/* void sbIDeviceRegistrar::registerDevice (in sbIDevice aDevice); */
NS_IMETHODIMP sbDeviceManager::RegisterDevice(sbIDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  // prevent anybody from seeing a half-added device
  nsAutoMonitor mon(mMonitor);

  nsresult rv;
  nsID* id;
  rv = aDevice->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(id);

  PRBool succeeded = mDevices.Put(*id, aDevice);
  NS_Free(id);
  if (!succeeded) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = aDevice->Connect();
  if (NS_FAILED(rv)) {
    // the device failed to connect, remove it from the hash
    mDevices.Remove(*id);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

/* void sbIDeviceRegistrar::unregisterDevice (in sbIDevice aDevice); */
NS_IMETHODIMP sbDeviceManager::UnregisterDevice(sbIDevice *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsID* id;
  rv = aDevice->GetId(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_ARG_POINTER(id);

  mDevices.Remove(*id);
  NS_Free(id);
  return NS_OK;
}

/* sbIDevice sbIDeviceRegistrar::getDevice (in nsIDPtr aDeviceId); */
NS_IMETHODIMP sbDeviceManager::GetDevice(const nsID * aDeviceId,
                                         sbIDevice **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG_POINTER(aDeviceId);

  if (!mMonitor) {
    // when EM_NO_RESTART is set, we don't see the appropriate app startup
    // attempt to manually initialize.
    nsresult rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool succeeded = mDevices.Get(*aDeviceId, _retval);
  return succeeded ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

/* void nsIObserver::observe (in nsISupports aSubject, in string aTopic, in wstring aData); */
NS_IMETHODIMP sbDeviceManager::Observe(nsISupports *aSubject,
                                       const char *aTopic,
                                       const PRUnichar *aData)
{
  nsresult rv;
  if (!strcmp(aTopic, APPSTARTUP_CATEGORY)) {
    // listen for profile startup and profile shutdown messages
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_PROFILE_STARTUP_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer,
                             SB_MAINWIN_PRESENTED_OBSERVER_ID,
                             PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_PROFILE_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

  } else if (!strcmp(NS_PROFILE_STARTUP_OBSERVER_ID, aTopic)) {
    // Called after the profile has been loaded, so prefs and such are available
    rv = this->Init();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (!strcmp(SB_MAINWIN_PRESENTED_OBSERVER_ID, aTopic)) {
    // Called after the main Songbird window is presented in case device
    // enumeration hangs.
    rv = BeginMarshallMonitoring();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (!strcmp(NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID, aTopic)) {
    // Called when the request to shutdown has been granted
    // We can't watch the -requested notification since it may not always be
    // fired :(. Due to Bug 9459 this will be called twice.
    rv = this->QuitApplicationGranted();
    NS_ENSURE_SUCCESS(rv, rv);

    // Remove the observer so we don't get called a second time, since we are
    // shutting down anyways this should not cause problems.
     nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_QUIT_APPLICATION_GRANTED_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

  } else if (!strcmp(SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, aTopic)) {
    // Called during shutdown, the profile is still available
    rv = this->PrepareShutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (!strcmp(NS_PROFILE_SHUTDOWN_OBSERVER_ID, aTopic)) {
    // Called when near the end of shutdown, profile is just about to be gone
    rv = this->FinalShutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    // remove all the observers
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_PROFILE_STARTUP_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, SB_MAINWIN_PRESENTED_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_PROFILE_SHUTDOWN_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult sbDeviceManager::Init()
{
  nsresult rv;

  NS_ENSURE_FALSE(mMonitor, NS_ERROR_ALREADY_INITIALIZED);

  mMonitor = nsAutoMonitor::NewMonitor(__FILE__);
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsAutoMonitor mon(mMonitor);

  // initialize the hashtables
  PRBool succeeded;
  succeeded = mControllers.Init();
  NS_ENSURE_TRUE(succeeded, NS_ERROR_OUT_OF_MEMORY);

  succeeded = mDevices.Init();
  NS_ENSURE_TRUE(succeeded, NS_ERROR_OUT_OF_MEMORY);

  succeeded = mMarshalls.Init();
  NS_ENSURE_TRUE(succeeded, NS_ERROR_OUT_OF_MEMORY);

  // load the marshalls
  nsCOMPtr<nsICategoryManager> catMgr =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = catMgr->EnumerateCategory("songbird-device-marshall",
                                 getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  rv = enumerator->HasMoreElements(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);

  while(hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = enumerator->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupportsCString> data = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString entryName;
    rv = data->GetData(entryName);
    NS_ENSURE_SUCCESS(rv, rv);

    char * contractId;
    rv = catMgr->GetCategoryEntry("songbird-device-marshall", entryName.get(), &contractId);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDeviceMarshall> marshall =
      do_CreateInstance(contractId, &rv);
    NS_Free(contractId);
    NS_ENSURE_SUCCESS(rv, rv);

    nsID* id;
    rv = marshall->GetId(&id);
    NS_ENSURE_SUCCESS(rv, rv);

    succeeded = mMarshalls.Put(*id, marshall);
    NS_Free(id);
    NS_ENSURE_TRUE(succeeded, NS_ERROR_OUT_OF_MEMORY);

    // have the marshall load the controllers
    nsCOMPtr<sbIDeviceControllerRegistrar> registrar =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbIDeviceControllerRegistrar*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = marshall->LoadControllers(registrar);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = enumerator->HasMoreElements(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // connect all the devices
  //rv = this->UpdateDevices();
  //NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbDeviceManager::GetCanDisconnect(PRBool* aCanDisconnect)
{
  NS_ENSURE_ARG_POINTER(aCanDisconnect);
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  nsAutoMonitor mon(mMonitor);

  nsresult rv;

  // get an array of our devices
  nsCOMPtr<nsIArray> devices;
  rv = GetDevices(getter_AddRefs(devices));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = devices->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  // for each of them, can we disconnect?
  PRBool canDisconnect = PR_TRUE;
  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<sbIDevice> device;
    rv = devices->QueryElementAt(i, NS_GET_IID(sbIDevice),
                                 getter_AddRefs(device));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = device->GetCanDisconnect(&canDisconnect);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!canDisconnect) {
      break;
    }
  }

  *aCanDisconnect = canDisconnect;

  return NS_OK;
}

nsresult sbDeviceManager::BeginMarshallMonitoring()
{
  nsresult rv;

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  nsAutoMonitor mon(mMonitor);

  // Get the list of marshalls.
  nsCOMPtr<nsIArray> marshalls;
  rv = this->GetMarshalls(getter_AddRefs(marshalls));
  NS_ENSURE_SUCCESS(rv, rv);

  // Begin monitoring for each marshall.
  PRUint32 length;
  rv = marshalls->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < length; i++) {
    // Get the next marshall.
    nsCOMPtr<sbIDeviceMarshall> marshall;
    rv = marshalls->QueryElementAt(i,
                                   NS_GET_IID(sbIDeviceMarshall),
                                   getter_AddRefs(marshall));
    if (NS_FAILED(rv)) {
      NS_WARNING("sbDeviceManager::BeginMarshallMonitoring: "
                 "marshall failed to QI to sbIDeviceMarshall");
      continue;
    }

    // Begin marshall monitoring, continue even if it fails
    rv = marshall->BeginMonitoring();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "sbDeviceManager::BeginMarshallMonitoring: "
                     "BeginMonitoring failed");
  }

  return NS_OK;
}

nsresult sbDeviceManager::QuitApplicationGranted()
{
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  nsAutoMonitor mon(mMonitor);

  nsresult rv;

  // There has been a request to shutdown, let's check with the devices and if
  // they are busy then query the user if they wish to force them to quit
  PRBool canDisconnect;
  rv = GetCanDisconnect(&canDisconnect);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!canDisconnect) {
    // one of our devices doesn't want to be disconnected
    nsCOMPtr<sbIPrompter> prompter =
      do_CreateInstance("@songbirdnest.com/Songbird/Prompter;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // This will hold up a dialog, we do not continue
    // until the dialog is closed, which will be closed automatically when
    // the devices are no longer busy, or the user closes it.
    nsCOMPtr<nsIDOMWindow> dialogWindow;
    prompter->OpenDialog
      (nsnull,
       NS_LITERAL_STRING("chrome://songbird/content/xul/waitForCompletion.xul"),
       NS_LITERAL_STRING("waitForCompletion"),
       NS_LITERAL_STRING(""),
       nsnull,
       getter_AddRefs(dialogWindow));
  }

  // Ok now we can shutdown
  return NS_OK;
}

nsresult sbDeviceManager::PrepareShutdown()
{
  nsresult rv;

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  nsAutoMonitor mon(mMonitor);

  // disconnect all the marshalls (i.e. stop watching for new devices)
  nsCOMPtr<nsIArray> marshalls;
  rv = this->GetMarshalls(getter_AddRefs(marshalls));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = marshalls->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<sbIDeviceMarshall> marshall;
    rv = marshalls->QueryElementAt(i, NS_GET_IID(sbIDeviceMarshall),
                                   getter_AddRefs(marshall));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = marshall->StopMonitoring();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
      "StopMonitoring returned an error, monitoring of devices may not be completely stopped.");
  }

  // ask the controllers to disconnect all devices
  nsCOMPtr<nsIArray> controllers;
  rv = this->GetControllers(getter_AddRefs(controllers));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = controllers->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<sbIDeviceController> controller;
    rv = controllers->QueryElementAt(i, NS_GET_IID(sbIDeviceController),
                                     getter_AddRefs(controller));
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to disconnect device.");
      continue;
    }
    rv = controller->DisconnectDevices();
    if (NS_FAILED(rv))
      NS_WARNING("Failed to disconnect device.");
  }

  return NS_OK;
}

nsresult sbDeviceManager::FinalShutdown()
{
  nsresult rv;

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  nsAutoMonitor mon(mMonitor);

  // get rid of all our controllers
  // ask the controllers to disconnect all devices
  nsCOMPtr<nsIArray> controllers;
  rv = this->GetControllers(getter_AddRefs(controllers));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = controllers->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<sbIDeviceController> controller;
    rv = controllers->QueryElementAt(i, NS_GET_IID(sbIDeviceController),
                                     getter_AddRefs(controller));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = controller->ReleaseDevices();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mControllers.Clear();
  mMarshalls.Clear();

  return NS_OK;
}

