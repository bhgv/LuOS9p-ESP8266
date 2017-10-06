#!/usr/bin/perl

foreach $ln (<>) {
    #print $ln;
    #if($ln=~m/U8G_FONT_SECTION\(\"u8g_/){
    if($ln=~m/U8G_FONT_SECTION\(\"u8g_([^\"]+)\"/){
    	print "    U8G_FONT_TABLE_ENTRY($1)\t\t\\\n";
    }
}

