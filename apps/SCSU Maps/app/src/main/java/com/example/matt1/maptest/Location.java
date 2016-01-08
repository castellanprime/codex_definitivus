package com.example.matt1.maptest;

/**
 * Created by Matt1 on 4/24/2015.
 */


public class Location {
    int id;
    double latitude;
    double longitude;
    String building;
    String buildingFullName;
    String roomNumber;
    boolean isBuilding;

    Location(double lat, double log, String build, String roomNum, String Fname,int i,boolean is)
    {
        id = i;
        latitude = lat;
        longitude = log;
        building = build;
        roomNumber = roomNum;
        buildingFullName = Fname;
        isBuilding = is;
    }


    int getId() {return id;}

    boolean getIsBuilding() {return isBuilding;}

    double getLatitude()
    {
        return latitude;
    }

    double getLongitude()
    {
        return longitude;
    }

    String getBuilding()
    {
        return building;
    }

    String getRoomNumber()
    {
        return roomNumber;
    }

    String getFullBuildingName() {return buildingFullName;}

}
