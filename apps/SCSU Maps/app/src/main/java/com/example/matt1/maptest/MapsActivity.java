package com.example.matt1.maptest;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.res.XmlResourceParser;
import android.graphics.Color;
import android.os.AsyncTask;
import android.support.v4.app.FragmentActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.PopupMenu;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.Marker;
import com.google.android.gms.maps.model.MarkerOptions;
import com.google.android.gms.maps.model.Polyline;
import com.google.android.gms.maps.model.PolylineOptions;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.List;


public class MapsActivity extends FragmentActivity implements GoogleMap.OnMarkerDragListener  {

    static GoogleMap mMap; // Might be null if Google Play services APK is not available.
    private List<Location> allLocations = new ArrayList<Location>(); //holds all locations
    private List<Event> userSchedule = new ArrayList<Event>(); //holds user class schedule
    private List<Event> eventFeed = new ArrayList<Event>(); //holds events
    private String userID = null;
    private boolean scheduleLoaded = false;
    private boolean eventsLoaded = false;
    private boolean buildingLoaded = false;
    private Location destination;
    private Marker endPoint = null;

    private Marker startPoint = null;
    static Polyline polylineToAdd;


    Button loginButton;
    Button popupButton;
    PopupMenu popupMenu;

    Button popupEvent;
    PopupMenu popupMenuEvent;

    //loads event from xml file
    void LoadEventFeed()
    {
        XmlResourceParser xrp = this.getResources().getXml(R.xml.eventfeed);

        Event temp;

        Location Holder;
        String locationID;
        String days;
        String time;
        String eventName;


        int idHold;

        try {
            while (xrp.getEventType() != XmlResourceParser.END_DOCUMENT) {
                if (xrp.getEventType() == XmlResourceParser.START_TAG) {
                    String s = xrp.getName();

                    if (s.equals("event")) {

                        s = xrp.getName();
                        days = xrp.getAttributeValue(null, "Day");
                        time = xrp.getAttributeValue(null, "Time");
                        eventName = xrp.getAttributeValue(null, "eventName");
                        locationID = xrp.getAttributeValue(null, "locationId");
                        idHold = Integer.parseInt(locationID);

                        Holder = getLocationByIndex(idHold - 1);

                        eventFeed.add(new Event(Holder, days, time, eventName));


                        days = "delay";

                    }


                }

                xrp.next();

            }
        } catch (Exception e) {
            Log.d("error", "error");
        }
    }

    //loads locations from xml file
    void LoadLocations() {

        XmlResourceParser xrp = this.getResources().getXml(R.xml.locations);


        String building;
        String roomnumber;
        String fullBuildingName;
        String lat;
        String lon;
        String id;
        double latHold;
        double lonHold;
        String isBuilding;
        boolean isBuildingHold;
        int idHold;

        try {
            while (xrp.getEventType() != XmlResourceParser.END_DOCUMENT) {
                if (xrp.getEventType() == XmlResourceParser.START_TAG) {
                    String s = xrp.getName();

                    if (s.equals("location")) {
                        //xrp.next();
                        s = xrp.getName();
                        building = xrp.getAttributeValue(null, "building");
                        fullBuildingName = xrp.getAttributeValue(null, "buildingFullName");
                        roomnumber = xrp.getAttributeValue(null, "roomNumber");
                        lat = xrp.getAttributeValue(null, "latitude");
                        lon = xrp.getAttributeValue(null, "longitude");
                        id = xrp.getAttributeValue(null, "id");
                        isBuilding = xrp.getAttributeValue(null,"isBuild");
                        isBuildingHold = Boolean.parseBoolean(isBuilding);
                        idHold = Integer.parseInt(id);
                        latHold = Double.parseDouble(lat);
                        lonHold = Double.parseDouble(lon);


                        allLocations.add(new Location(latHold, lonHold, building, roomnumber,fullBuildingName, idHold,isBuildingHold));

                    }

                }
                xrp.next();

            }
        } catch (Exception e) {
            Log.d("error", "error");
        }
    }

   //loads userSchedule using userID
    void LoadSchedule() {
        XmlResourceParser xrp = this.getResources().getXml(R.xml.d2l);

        Event temp;

        Location Holder;
        String locationID;
        String days;
        String time;
        String eventName;


        int idHold;

        try {
            while (xrp.getEventType() != XmlResourceParser.END_DOCUMENT) {
                if (xrp.getEventType() == XmlResourceParser.START_TAG) {
                    String s = xrp.getName();

                    if (s.equals("schedule")) {
                        if (xrp.getAttributeValue(null, "id").equals(userID)) {
                            s = xrp.getName();
                            days = xrp.getAttributeValue(null, "Day");
                            time = xrp.getAttributeValue(null, "Time");
                            eventName = xrp.getAttributeValue(null, "eventName");
                            locationID = xrp.getAttributeValue(null, "locationId");
                            idHold = Integer.parseInt(locationID);

                            Holder = getLocationByIndex(idHold - 1);

                            userSchedule.add(new Event(Holder, days, time, eventName));


                            days = "delay";

                        }


                    }

                }
                xrp.next();

            }
        } catch (Exception e) {
            Log.d("error", "error");
        }
    }

    //gets location by index from allLocations
    Location getLocationByIndex(int index) {
        return allLocations.get(index);
    }

    //loads buildings into popup menu
    void insertBuildingsIntoPopUp()
    {
        for (int i = 0; i < allLocations.size(); i++)
        {
            if(allLocations.get(i).isBuilding)
                popupMenu.getMenu().add(0,0,0,allLocations.get(i).buildingFullName);
        }
    }

    //loads user class schedule into popup menu
    void insertScheduleIntoPopUp() {
        for (int i = 0; i < userSchedule.size(); i++) {
            popupMenu.getMenu().add(userSchedule.get(i).eventName);
        }

    }

    //loads events into popup menu
    void insertEventsIntoPopUp() {
        for (int i = 0; i < eventFeed.size(); i++) {
            popupMenuEvent.getMenu().add(eventFeed.get(i).eventName);
        }

    }

    //checks if userID input is in d2l xml file
    boolean validateStudent(String user) {
        XmlResourceParser xrp = this.getResources().getXml(R.xml.d2l);


        try {
            while (xrp.getEventType() != XmlResourceParser.END_DOCUMENT) {
                if (xrp.getEventType() == XmlResourceParser.START_TAG) {
                    String s = xrp.getName();

                    if (s.equals("schedule")) {
                        if (xrp.getAttributeValue(null, "id").equals(user)) {
                            return true;
                        }
                    }
                }
                xrp.next();

            }
        } catch (Exception e) {
            Log.d("error", "error");
        }
        return false;
    }

    //displays login menu with title and message
    void DisplayLogin(String title, String msg) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(title);
        builder.setMessage(msg);

        final EditText input = new EditText(this);
        // input.setId(TEXT_ID);
        builder.setView(input);

        builder.setPositiveButton("StarID Login", new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
                userID = input.getText().toString();
                if (validateStudent(userID)) {
                    setLoginButtonText();
                    return;
                }
                else
                    DisplayLogin("Student Not Found Try Again", "Enter StarID:");
            }
        });

        builder.setNegativeButton("Guest Login", new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                userID = "guest";
                return;
            }
        });

        builder.show();
    }

    //display event information in AlertDialog
    void DisplayEventInfo(String title, String msg, Location loc) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(title);
        builder.setMessage(msg);
        final Location temp = loc;

        builder.setPositiveButton("Plot Point", new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
                addMarker(mMap,temp);
                return;
            }
        });

        builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                return;
            }
        });

        builder.show();
    }


    //functions for moving map marker
    @Override
    public void onMarkerDragStart(Marker marker) {
        if (polylineToAdd != null)
            polylineToAdd.remove();
    }

    @Override
    public void onMarkerDrag(Marker marker) {
    }

    @Override
    public void onMarkerDragEnd (Marker marker)
    {
        startPoint = marker;
    }
    //end of marker functions


    //creates user interface sets up applications loading in locations
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_maps);
        setUpMapIfNeeded();
        mMap.moveCamera(CameraUpdateFactory.newLatLng(new LatLng(45.552987, -94.151073)));
        mMap.moveCamera(CameraUpdateFactory.zoomTo(16.0f)); //16

        DisplayLogin("Login:", "Enter StarID:");



        startPoint = mMap.addMarker(new MarkerOptions().position(new LatLng(45.552987, -94.151073)).title("You are here").draggable(true));
        mMap.setOnMarkerDragListener(this);


        loginButton = (Button) findViewById(R.id.login);

        popupButton = (Button) findViewById(R.id.button_popup);
        popupButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                popupMenu = new PopupMenu(getApplicationContext(), v);
                buildingLoaded = false;

                popupMenu.inflate(R.menu.popupmenu1);

                if (scheduleLoaded)
                    insertScheduleIntoPopUp();




                popupMenu.show();

                popupMenu.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {


                    private int popupmenu1;

                    @Override
                    public boolean onMenuItemClick(MenuItem item) {

                        for (int i = 0; i < userSchedule.size(); i++) {
                            if (item.toString().equals(userSchedule.get(i).getEventName())) {
                                destination = userSchedule.get(i).getPlace();
                                //addMarker(mMap, destination);
                                DisplayEventInfo("Event Information",userSchedule.get(i).getEventInfo(),destination);
                                //Toast.makeText(getApplicationContext(), userSchedule.get(i).getEventInfo(), Toast.LENGTH_SHORT).show();
                            }
                        }

                        for (int i = 0; i < allLocations.size(); i++) {
                            if (item.toString().equals(allLocations.get(i).getFullBuildingName()) && allLocations.get(i).getIsBuilding()) {
                                destination = allLocations.get(i);
                                addMarker(mMap, destination);
                                Toast.makeText(getApplicationContext(), destination.getFullBuildingName(), Toast.LENGTH_SHORT).show();
                            }
                        }


                        if(item.toString().equals("Show Buildings"))
                        {
                            Toast.makeText(getApplicationContext(), item.toString(), Toast.LENGTH_SHORT).show();

                            if(!buildingLoaded) {
                                buildingLoaded = true;
                                insertBuildingsIntoPopUp();
                            }
                            popupMenu.show();
                        }


                        if (item.toString().equals("Load schedule") && !scheduleLoaded && userID != "guest") {
                            Toast.makeText(getApplicationContext(), item.toString(), Toast.LENGTH_SHORT).show();
                            LoadSchedule();


                            insertScheduleIntoPopUp();


                            scheduleLoaded = true;
                            popupMenu.show();
                        }


                        return false;
                    }
                });
            }
        });



        popupEvent = (Button) findViewById(R.id.Eventbutton);
        popupEvent.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                popupMenuEvent = new PopupMenu(getApplicationContext(), v);


                popupMenuEvent.inflate(R.menu.popupmenuevents);

                if (eventsLoaded)
                    insertEventsIntoPopUp();


                popupMenuEvent.show();




                popupMenuEvent.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {


                    @Override
                    public boolean onMenuItemClick(MenuItem item) {

                        for (int i = 0; i < eventFeed.size(); i++) {
                            if (item.toString().equals(eventFeed.get(i).getEventName())) {
                                destination = eventFeed.get(i).getPlace();
                                //addMarker(mMap, destination);
                                DisplayEventInfo("Event Information",eventFeed.get(i).getEventInfo(),destination);
                                //Toast.makeText(getApplicationContext(), eventFeed.get(i).getEventInfo(), Toast.LENGTH_SHORT).show();
                            }
                        }


                        if (item.toString().equals("Load Events") && !eventsLoaded) {
                            Toast.makeText(getApplicationContext(), item.toString(), Toast.LENGTH_SHORT).show();
                            LoadEventFeed();


                            insertEventsIntoPopUp();

                            eventsLoaded = true;
                            popupMenuEvent.show();
                        }






                        return false;
                    }
                });
            }
        });

        setLoginButtonText();

        LoadLocations();



        // AlertDialog.Builder dialog = new AlertDialog.Builder(this);
        //dialog.setTitle("TEST TITLE");
        //dialog.setMessage("MSG TEST");
        //dialog.show();


    }


    //sets login/logout button text
    public void setLoginButtonText()
    {
        if(userID == "guest" || userID == null)
        {
            loginButton.setText("login");
        }
        else
        {
            loginButton.setText("logout");
        }
    }


    @Override
    protected void onResume() {
        super.onResume();
        setUpMapIfNeeded();
    }

    //adds marker to map based on location
    public void addMarker(GoogleMap map, Location e) {
        double lat = e.getLatitude();
        double lng = e.getLongitude();
        String title = e.getFullBuildingName() + " " + e.getRoomNumber();

        if (endPoint != null)
            endPoint.remove();

        endPoint = map.addMarker(new MarkerOptions().position(new LatLng(lat, lng)).title(title));


        if (polylineToAdd != null)
            polylineToAdd.remove();

        String urlString=
                "http://maps.googleapis.com/maps/api/directions/json?origin="
                        + startPoint.getPosition().latitude + "," +  startPoint.getPosition().longitude  +"&destination="
                        + destination.getLatitude() + "," + destination.getLongitude() + "&mode=walking";



        new MyDownloadTask().execute(urlString);



    }


    //displays login allows user to login in again
    public void loginOnClick(View v)
    {
        TextView test = (TextView) v;
        if(test.getText() == "login")
        {
            if (endPoint != null)
                endPoint.remove();

            if (polylineToAdd != null)
                polylineToAdd.remove();

            DisplayLogin("Login:", "Enter StarID:");
        }
        else
        {
            Toast.makeText(getApplicationContext(), "Logout", Toast.LENGTH_SHORT).show();
            userID = "guest";
            scheduleLoaded = false;
            eventsLoaded = false;
            userSchedule.clear();
            eventFeed.clear();
            if (endPoint != null)
                endPoint.remove();

            if (polylineToAdd != null)
                polylineToAdd.remove();
            setLoginButtonText();
        }
    }

    //resets the map to campus view
    public void ResetOnClick(View v) {
        mMap.moveCamera(CameraUpdateFactory.newLatLng(new LatLng(45.552987, -94.151073)));
        mMap.moveCamera(CameraUpdateFactory.zoomTo(16.0f)); //16
    }




    /**
     * Sets up the map if it is possible to do so (i.e., the Google Play services APK is correctly
     * installed) and the map has not already been instantiated.. This will ensure that we only ever
     * call {@link #setUpMap()} once when {@link #mMap} is not null.
     * <p/>
     * If it isn't installed {@link SupportMapFragment} (and
     * {@link com.google.android.gms.maps.MapView MapView}) will show a prompt for the user to
     * install/update the Google Play services APK on their device.
     * <p/>
     * A user can return to this FragmentActivity after following the prompt and correctly
     * installing/updating/enabling the Google Play services. Since the FragmentActivity may not
     * have been completely destroyed during this process (it is likely that it would only be
     * stopped or paused), {@link #onCreate(Bundle)} may not be called again so we should call this
     * method in {@link #onResume()} to guarantee that it will be called.
     */
    private void setUpMapIfNeeded() {
        // Do a null check to confirm that we have not already instantiated the map.
        if (mMap == null) {
            // Try to obtain the map from the SupportMapFragment.
            mMap = ((SupportMapFragment) getSupportFragmentManager().findFragmentById(R.id.map))
                    .getMap();
            // Check if we were successful in obtaining the map.
            if (mMap != null) {
                setUpMap();
            }
        }
    }

    /**
     * This is where we can add markers or lines, add listeners or move the camera. In this case, we
     * just add a marker near Africa.
     * <p/>
     * This should only be called once and when we are sure that {@link #mMap} is not null.
     */
    private void setUpMap() {
        mMap.addMarker(new MarkerOptions().position(new LatLng(0, 0)).title("Marker"));
    }


    //displays map pathing
    class MyDownloadTask extends AsyncTask<String, Integer, List<LatLng>> {



        @Override
        protected void onPreExecute() {
            super.onPreExecute();
        }

        protected List<LatLng> doInBackground(String... string) {
            String returnValue ="";
            List<LatLng> lines = new ArrayList<LatLng>();

            try {
                URL url = new URL(string[0]);
                URLConnection urlConnection = url.openConnection();
                BufferedReader in = new BufferedReader(new InputStreamReader(
                        urlConnection.getInputStream()));
                StringBuilder sb = new StringBuilder();
                for (String line = in.readLine(); line != null; line =in.readLine()){
                    sb.append(line);
                }
                returnValue = sb.toString();
            }
            catch (Exception e)
            {
                returnValue = e.toString();
            }

            try {
                JSONObject result = new JSONObject(returnValue);
                JSONArray routes = result.getJSONArray("routes");

                long distanceForSegment = routes.getJSONObject(0).getJSONArray("legs").getJSONObject(0).getJSONObject("distance").getInt("value");

                JSONArray steps = routes.getJSONObject(0).getJSONArray("legs")
                        .getJSONObject(0).getJSONArray("steps");



                for (int i = 0; i < steps.length(); i++) {
                    String polyline = steps.getJSONObject(i).getJSONObject("polyline").getString("points");

                    for (LatLng p : decodePolyline(polyline)) {
                        lines.add(p);
                    }
                }


            }
            catch (Exception e)
            {
                String abc = e.toString();
            }

            return lines;

        }

        protected void onProgressUpdate(String... progress) {
        }

        private List<LatLng> decodePolyline(String encoded) {

            List<LatLng> poly = new ArrayList<LatLng>();

            int index = 0, len = encoded.length();
            int lat = 0, lng = 0;

            while (index < len) {
                int b, shift = 0, result = 0;
                do {
                    b = encoded.charAt(index++) - 63;
                    result |= (b & 0x1f) << shift;
                    shift += 5;
                } while (b >= 0x20);
                int dlat = ((result & 1) != 0 ? ~(result >> 1) : (result >> 1));
                lat += dlat;

                shift = 0;
                result = 0;
                do {
                    b = encoded.charAt(index++) - 63;
                    result |= (b & 0x1f) << shift;
                    shift += 5;
                } while (b >= 0x20);
                int dlng = ((result & 1) != 0 ? ~(result >> 1) : (result >> 1));
                lng += dlng;

                LatLng p = new LatLng((double) lat / 1E5, (double) lng / 1E5);
                poly.add(p);
            }

            return poly;
        }

        @Override
        protected void onPostExecute(List<LatLng> lines) {
            polylineToAdd = mMap.addPolyline(new PolylineOptions().addAll(lines).width(3).color(Color.RED));
        }
    }


}

