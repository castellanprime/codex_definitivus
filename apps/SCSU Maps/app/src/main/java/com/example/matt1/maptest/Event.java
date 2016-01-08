package com.example.matt1.maptest;

/**
 * Created by Matt1 on 4/25/2015.
 */
public class Event {
    Location place;
    String days;
    String time;
    String eventName;

    Event(Location p, String d, String t, String n)
    {
        place = p;
        days = d;
        time = t;
        eventName = n;
    }

    String getEventInfo()
    {
        if(place.roomNumber.equals("")) {
            String tempName = String.format("Event: " + eventName + "\n");
            String tempPlace = String.format("Location: " + place.getFullBuildingName() + "\n");
            String tempDay = String.format("Date: " + days + "\n" + "Time: " + time);
            return tempName + tempPlace + tempDay;
        }
        else
        {
            String tempName = String.format("Event: " + eventName + "\n");
            String tempPlace = String.format("Location: " + place.getFullBuildingName() + "\n" +"Room Number: "+ place.getBuilding() + " " + place.getRoomNumber() + "\n");
            String tempDay = String.format("Day: " + days + "\n" + "Time: " + time);
            return tempName + tempPlace + tempDay;
        }
    }

    Location getPlace()
    {
        return place;
    }

    String getDays()
    {
        return days;
    }

    String getTime()
    {
        return time;
    }

    String getEventName()
    {
        return eventName;
    }



}
