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

try
{

/**
 * ----------------------------------------------------------------------------
 * Global Utility Functions (it's fun copying them everywhere...)
 * ----------------------------------------------------------------------------
 */

/**
 * Logs a string to the error console. 
 * @param   string
 *          The string to write to the error console..
 */  
function LOG(string) {
  dump("***CoreQT*** " + string + "\n");
} // LOG

/**
 * Dumps an object's properties to the console
 * @param   obj
 *          The object to dump
 * @param   objName
 *          A string containing the name of obj
 */  
function listProperties(obj, objName) {
  var columns = 1;
  var count = 0;
  var result = "";
  for (var i in obj) {
      result += objName + "." + i + " = " + obj[i] + "\t\t\t";
      count = ++count % columns;
      if ( count == columns - 1 )
        result += "\n";
  }
  LOG("listProperties");
  dump(result + "\n");
}

/**
 * Converts seconds to a time string
 * @param   seconds
 *          The number of seconds to convert
 * @return  A string containing the converted time
 */  
function EmitSecondsToTimeString( seconds )
{
  if ( seconds < 0 )
    return "00:00";
  seconds = parseFloat( seconds );
  var minutes = parseInt( seconds / 60 );
  seconds = parseInt( seconds ) % 60;
  var hours = parseInt( minutes / 60 );
  if ( hours > 50 ) // lame
    return "Error";
  minutes = parseInt( minutes ) % 60;
  var text = ""
  if ( hours > 0 )
  {
    text += hours + ":";
  }
  if ( minutes < 10 )
  {
    text += "0";
  }
  text += minutes + ":";
  if ( seconds < 10 )
  {
    text += "0";
  }
  text += seconds;
  return text;
}

// The QT ActiveX Control Wrapper
function CoreQT()
{
	this._object = null;
	
	this._url = "";
	
  this._playing = false;
  
  this._paused  = false;

  /**
   *
   */
  this._verifyObject = function() {
    // Like, actually verify that we think it's a wmp object.  Not just that it's defined.
    if ( (this._object === undefined) || ( ! this._object ) || ( ! this._object.uiMode ) )
    {
      LOG("VERIFY OBJECT FAILED.  OBJECT DOES NOT HAVE PROPER WMP API.")
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }
  }  
  
  // When another core wrapper takes over, stop.
  this.onSwappedOut = function onSwappedOut()
  {
  	this._verifyObject();
    this._object.stop();
    this._object.Rewind();
  } 
  
  // Set the url and tell it to just play it.  Eventually this talks to the media library object to make a temp playlist.
  this.playUrl = function ( url )
  {
  	this._verifyObject();

    if (!url)
      throw Components.results.NS_ERROR_INVALID_ARG;

    LOG( "Trying to play url: " + url );
    this._url = url;

    this._url.SetURL( url );
    setTimeout('gQTCore.play()', 250);
  }

  this.play      = function ()
  {
  	this._verifyObject();
  	
    this._paused = false;
    this._playing = true;
    
    return this._object.Play();
  }
  
  this.pause     = function ()
  {
  	this._verifyObject();

		if( this._object._paused )
		  return this._object.play();
    
    this._object.m_Paused = true;
    this._object.m_Playing = false;

    return this._object.Stop();
  }
  
  this.stop = function ()
  {
    this._verifyObject();
    
    this._object._paused = false;
    this._object._playing = false;
    
    this._object.Stop();
    return this._object.Rewind();
  }
  
  this.next = function ()
  {
  	this._verifyObject();
    return CNullStub();
  }
  
  this.prev = function ()
  {
  	this._verifyObject();
    return CNullStub();
  }
  
  this.getPlaying   = function ()
  {
  	this._verifyObject();
    return this._object._playing;
  }
  
  this.getPaused    = function ()
  {
  	this._verifyObject();
    return this._object._paused;
  }
  
  this.getLength = function ()
  {
  	this._verifyObject();
    return this._object.GetEndTime();
  }
  
  this.getPosition = function ()
  {
  	this._verifyObject();
    return this._object.GetTime();
  }
  
  this.setPosition = function ( pos )
  {
  	this._verifyObject();
    this._object.SetTime( pos );
    this._object.doPlay();
  }
  
  this.getVolume   = function ()
  {
    return ( this._object.GetVolume() );
  }
  
  this.setVolume   = function ( vol )
  {
  	this._verifyObject();
    this._object.SetVolume( vol );
  }
  
  this.getMute     = function ()
  {
    return this._object.GetMute();
  }
  
  this.setMute     = function ( mute )
  {
  	this._verifyObject();
    this._object.SetMute( mute );
  }
  
  /**
   *
   */
  this.getId = function() {
    return this._id;
  }
 
  this.setId = function(id) {
    if (this._id != id)
      this._id = id;
  }
 
  /**
   *
   */
  this.getObject = function() {
    return this._object;
  }
  
  this.setObject = function(object) {
   if (this._object != object) {
      if (this._object)
        this.swapCore();
      this._object = object;
    }
  }
  
  /**
   * See nsISupports.idl
   */
  this.QueryInterface = function(iid) {
    if (!iid.equals(Components.interfaces.sbICoreWrapper) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }

}

/**
 * ----------------------------------------------------------------------------
 * Global variables and autoinitialization.
 * ----------------------------------------------------------------------------
 */
var gQTCore = new CoreQT();

/**
  * This is the function called from a document onload handler to bind everything as playback.
  * The <html:object>s won't have their scriptable APIs attached until the onload.
  */
function CoreQTDocumentInit( id )
{
  try
  {
    var gPPS = Components.classes["@songbird.org/Songbird/PlaylistPlayback;1"]
                         .getService(Components.interfaces.sbIPlaylistPlayback);
    var theQTInstance = document.getElementById( id );

    gCoreQT.setId("QT1");
    gCoreQT.setObject(theQTInstance);
    gPPS.addCore(gCoreQT, true);

  }
  catch ( err )
  {
    LOG( "\n!!! coreQT failed to bind properly\n" + err );
  }
} 
 
}
catch ( err )
{
  LOG( "\n!!! coreQT failed to load properly\n" + err );
}