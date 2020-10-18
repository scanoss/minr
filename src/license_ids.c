#define MAX_LICENSE_ID 64
#define MAX_LICENSE_TEXT 1024

typedef struct normalized_license
{
  char spdx_id[MAX_LICENSE_ID];
  char text[MAX_LICENSE_TEXT];
  int ln;
} normalized_license;

normalized_license licenses[38];

int load_licenses()
{
  strcpy(licenses[0].spdx_id, "MPL-1.1");
  strcpy(licenses[0].text,"thecontentsofthisfilearesubjecttothemozillapubliclicenseversion11thelicenseyoumaynotusethisfileexceptincompliancewiththelicense");
  licenses[0].ln = 127;

  strcpy(licenses[1].spdx_id, "GPL-3.0-only");
  strcpy(licenses[1].text,"youcanredistributeitandormodifyitunderthetermsofthegnugeneralpubliclicenseversion3aspublishedbythefreesoftwarefoundation");
  licenses[1].ln = 120;

  strcpy(licenses[2].spdx_id, "GPL-3.0-only");
  strcpy(licenses[2].text,"youcanredistributeitandormodifyitunderthetermsofthegnugeneralpubliclicenseaspublishedbythefreesoftwarefoundationversion3thisprogram");
  licenses[2].ln = 131;

  strcpy(licenses[3].spdx_id, "BSD-3-Clause");
  strcpy(licenses[3].text,"northenamesofitscontributorsmaybeusedtoendorseorpromoteproductsderivedfromthissoftwarewithoutspecificpriorwrittenpermission");
  licenses[3].ln = 123;

  strcpy(licenses[4].spdx_id, "GPL-2.0-or-later");
  strcpy(licenses[4].text,"youcanredistributeitandormodifyitunderthetermsofthegnugeneralpubliclicenseaspublishedbythefreesoftwarefoundationeitherversion2");
  licenses[4].ln = 126;

  strcpy(licenses[5].spdx_id, "CPL-1.0");
  strcpy(licenses[5].text,"commonpubliclicense");
  licenses[5].ln = 19;

  strcpy(licenses[6].spdx_id, "LGPL-2.1-or-later");
  strcpy(licenses[6].text,"youcanredistributeitandormodifyitunderthetermsofthegnulibrarygeneralpubliclicenseaspublishedbythefreesoftwarefoundationeitherversion2");
  licenses[6].ln = 133;

  strcpy(licenses[7].spdx_id, "LGPL-2.1-or-later");
  strcpy(licenses[7].text,"youcanredistributeitandormodifyitunderthetermsofthegnugeneralpubliclicenseaspublishedbythefreesoftwarefoundationeitherversion21");
  licenses[7].ln = 127;

  strcpy(licenses[8].spdx_id, "LGPL-2.1-or-later");
  strcpy(licenses[8].text,"youcanredistributeitandormodifyitunderthetermsofthegnulessergeneralpubliclicenseaspublishedbythefreesoftwarefoundationeitherversion2");
  licenses[8].ln = 132;

  strcpy(licenses[9].spdx_id, "EPL-1.0");
  strcpy(licenses[9].text,"eclipsepubliclicensev10");
  licenses[9].ln = 23;

  strcpy(licenses[10].spdx_id, "AGPL-3.0-or-later");
  strcpy(licenses[10].text,"gnuafferogeneralpubliclicensegnuagplaspublishedbythefreesoftwarefoundationeitherversion3ofthelicenseoratyouroptionanylaterversion");
  licenses[10].ln = 129;

  strcpy(licenses[11].spdx_id, "AGPL-3.0-or-later");
  strcpy(licenses[11].text,"thisprogramisfreesoftwareyoucanredistributeitandormodifyitunderthetermsofthegnuafferogeneralpubliclicenseaspublishedbythefreesoftwarefoundationeitherversion3ofthelicenseoratyouroptionanylaterversion");
  licenses[11].ln = 198;

  strcpy(licenses[12].spdx_id, "MIT");
  strcpy(licenses[12].text,"thesoftwareisprovidedasiswithoutwarrantyofanykindexpressorimpliedincludingbutnotlimitedtothewarrantiesofmerchantabilityfitnessforaparticularpurposeandnoninfringementinnoeventshalltheauthorsorcopyrightholdersbeliableforanyclaimdamagesorotherliabilitywhetherinanactionofcontracttortorotherwisearisingfromoutoforinconnectionwiththesoftwareortheuseorotherdealingsinthesoftware");
  licenses[12].ln = 372;

  strcpy(licenses[13].spdx_id, "MIT");
  strcpy(licenses[13].text,"mitlicense");
  licenses[13].ln = 10;

  strcpy(licenses[14].spdx_id, "MIT");
  strcpy(licenses[14].text,"permissiontousecopymodifyanddistributethissoftwareforanypurposewithorwithoutfeeisherebygrantedprovidedthattheabovecopyrightnoticeandthispermissionnoticeappearinallcopies");
  licenses[14].ln = 169;

  strcpy(licenses[15].spdx_id, "MIT");
  strcpy(licenses[15].text,"todealinthesoftwarewithoutrestrictionincludingwithoutlimitationtherightstousecopymodifymergepublishdistributesublicenseandorsellcopiesofthesoftwareandtopermitpersonstowhomthesoftwareisfurnishedtodososubjecttothefollowingconditionstheabovecopyrightnoticeandthispermissionnotice");
  licenses[15].ln = 276;

  strcpy(licenses[16].spdx_id, "MPL-2.0");
  strcpy(licenses[16].text,"thissourcecodeformissubjecttothetermsofthemozillapubliclicensev20ifacopyofthemplwasnotdistributedwiththisfileyoucanobtainoneathttpmozillaorgmpl20");
  licenses[16].ln = 145;

  strcpy(licenses[17].spdx_id, "AGPL-3.0");
  strcpy(licenses[17].text,"thisprogramisfreesoftwareyoucanredistributeitandormodifyitunderthetermsofthegnuafferogeneralpubliclicenseaspublishedbythefreesoftwarefoundationversion3thisprogramisdistributedinthehopethatitwillbeusefulbutwithoutanywarrantywithouteventheimpliedwarrantyofmerchantabilityorfitnessforaparticularpurposeseethegnuafferogeneralpubliclicenseformoredetails");
  licenses[17].ln = 348;

  strcpy(licenses[18].spdx_id, "AGPL-3.0");
  strcpy(licenses[18].text,"httpwwwgnuorglicensesagpl30txt");
  licenses[18].ln = 30;

  strcpy(licenses[19].spdx_id, "LGPL-3.0-or-later");
  strcpy(licenses[19].text,"youcanredistributeitandormodifyitunderthetermsofthegnulessergeneralpubliclicenseaspublishedbythefreesoftwarefoundationeitherversion3");
  licenses[19].ln = 132;

  strcpy(licenses[20].spdx_id, "BSD-2-Clause");
  strcpy(licenses[20].text,"redistributionsofsourcecodemustretaintheabovecopyrightnoticethislistofconditionsandthefollowingdisclaimer2redistributionsinbinaryformmustreproducetheabovecopyrightnoticethislistofconditionsandthefollowingdisclaimerinthedocumentationandorothermaterialsprovidedwiththedistribution");
  licenses[20].ln = 278;

  strcpy(licenses[21].spdx_id, "MS-PL");
  strcpy(licenses[21].text,"microsoftpubliclicense");
  licenses[21].ln = 22;

  strcpy(licenses[22].spdx_id, "GPL-2.0-only");
  strcpy(licenses[22].text,"youcanredistributeitandormodifyitunderthetermsversion2ofthegnugeneralpubliclicenseaspublishedbythefreesoftwarefoundation");
  licenses[22].ln = 120;

  strcpy(licenses[23].spdx_id, "GPL-2.0-only");
  strcpy(licenses[23].text,"youcanredistributeitandormodifyitunderthetermsofversion2ofthegnugeneralpubliclicenseaspublishedbythefreesoftwarefoundation");
  licenses[23].ln = 122;

  strcpy(licenses[24].spdx_id, "GPL-2.0-only");
  strcpy(licenses[24].text,"youcanredistributeitandormodifyitunderthetermsofthegnugeneralpubliclicenseversion2onlyaspublishedbythefreesoftwarefoundation");
  licenses[24].ln = 124;

  strcpy(licenses[25].spdx_id, "GPL-2.0-only");
  strcpy(licenses[25].text,"youcanredistributeitandormodifyitunderthetermsofthegnugeneralpubliclicenseversion2aspublishedbythefreesoftwarefoundation");
  licenses[25].ln = 120;

  strcpy(licenses[26].spdx_id, "GPL-2.0-only");
  strcpy(licenses[26].text,"httpwwwgnuorglicensesgpl20html");
  licenses[26].ln = 30;

  strcpy(licenses[27].spdx_id, "GPL-2.0-only");
  strcpy(licenses[27].text,"ismadeavailabletoanyonewishingtousemodifycopyorredistributeitsubjecttothetermsandconditionsofthegnugeneralpubliclicensev20");
  licenses[27].ln = 122;

  strcpy(licenses[28].spdx_id, "GPL-2.0-only");
  strcpy(licenses[28].text,"youcanredistributeitandormodifyitunderthetermsofthegnugeneralpubliclicenseaspublishedbythefreesoftwarefoundationversion2thisprogram");
  licenses[28].ln = 131;

  strcpy(licenses[29].spdx_id, "PostgreSQL");
  strcpy(licenses[29].text,"permissiontousecopymodifyanddistributethissoftwareanditsdocumentationforanypurposewithoutfeeandwithoutawrittenagreementisherebygrantedprovidedthattheabovecopyrightnoticeandthisparagraphandthefollowingtwoparagraphsappearinallcopiesinnoeventshall");
  licenses[29].ln = 244;

  strcpy(licenses[30].spdx_id, "PostgreSQL");
  strcpy(licenses[30].text,"thissoftwareisreleasedunderthepostgresqllicence");
  licenses[30].ln = 47;

  strcpy(licenses[31].spdx_id, "Apache-2.0");
  strcpy(licenses[31].text,"httpwwwapacheorglicenseslicense20");
  licenses[31].ln = 33;

  strcpy(licenses[32].spdx_id, "Apache-2.0");
  strcpy(licenses[32].text,"apachelicenseversion20");
  licenses[32].ln = 22;

  strcpy(licenses[33].spdx_id, "FSFULLR");
  strcpy(licenses[33].text,"freesoftwarefoundationgivesunlimitedpermissiontocopyandordistributeitwithorwithoutmodificationsaslongasthisnoticeispreserved");
  licenses[33].ln = 124;

  strcpy(licenses[34].spdx_id, "WTFPL");
  strcpy(licenses[34].text,"dowhatthefuckyouwanttopubliclicense");
  licenses[34].ln = 35;

  strcpy(licenses[35].spdx_id, "GPL-3.0-or-later");
  strcpy(licenses[35].text,"youcanredistributeitandormodifyitunderthetermsofthegnulessergeneralpubliclicenseaspublishedbythefreesoftwarefoundationeitherversion3");
  licenses[35].ln = 132;

  strcpy(licenses[36].spdx_id, "Zlib");
  strcpy(licenses[36].text,"1theoriginofthissoftwaremustnotbemisrepresentedyoumustnotclaimthatyouwrotetheoriginalsoftwareifyouusethissoftwareinaproductanacknowledgmentintheproductdocumentationwouldbeappreciatedbutisnotrequired2alteredsourceversionsmustbeplainlymarkedassuchandmustnotbemisrepresentedasbeingtheoriginalsoftware3thisnoticemaynotberemovedoralteredfromanysourcedistribution");
  licenses[36].ln = 357;

  strcpy(licenses[37].spdx_id, "IPL-1.0");
  strcpy(licenses[37].text,"ibmpubliclicense");
  licenses[37].ln = 16;


  return 38;
}
