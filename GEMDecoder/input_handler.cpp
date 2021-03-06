/*
 * input_handler.cpp
 *
 *  Created on: Nov 22, 2016
 *      Author: newdriver
 */

#include <stdint.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include "string"

#include <TThread.h>
// root function
#include <TTree.h>
#include <TCanvas.h>
#include "TRootEmbeddedCanvas.h"
//user defined functions
#include "../GEMDecoder/input_handler.h"



#define ZERO_CONPRESION_SIGMA_THR  4

// namespace
using namespace std;
using namespace evio;


InputHandler::InputHandler(string str) {
	// TODO Auto-generated constructor stub
	  filename = str;
	  //file.open(filename.c_str(), ios::in|ios::binary);
	  fRawDecoder = 0;
	  mAPVRawHistos.clear();
	  mAPVRawTSs.clear();
	  vSRSSingleEventData.clear();// vector parameter
}

std::string InputHandler::SetMapping(string mappingfile){
	if(mappingfile.empty()){
		return vDefaultMappingPath;
	}else{
		vDefaultMappingPath=mappingfile;
		return vDefaultMappingPath;
	}
}

int InputHandler::RawProcessAllEvents(int entries){

	int entry = 0;
	vector<TH1F*> RawAPVs;
	TCanvas *cRaw = new TCanvas("cRaw", "APV Raw Signal", 10, 10, 1000, 800);
	cRaw->Divide(5,5);

	try{
		evioFileChannel chan(filename.c_str(), "r");
		chan.open();
		while(chan.read()&&entry<entries) {
			map<int, map<int, map<int, vector<int> > > > mmHit;
			//vector<uint32_t> srsSingleEvent;
			vSRSSingleEventData.clear();

			evioDOMTree event(chan);
			evioDOMNodeListP mpdEventList = event.getNodeList( isLeaf() );
			cout<<"number of banks: "<<mpdEventList->size()<<endl;            ///what is bank number?
			int n_bank = mpdEventList->size();
			cout<<n_bank<<endl;
			//if(n_bank)
			evioDOMNodeList::iterator iter;
			for(iter=mpdEventList->begin(); iter!=mpdEventList->end(); ++iter) {

				cout<<"bank #:"<< (*iter)->tag<<endl;
				if((*iter)->tag == 10) {
					vector<uint32_t> *vec = (*iter)->getVector<uint32_t>();
					if(vec!=NULL) {
						vSRSSingleEventData.reserve(vSRSSingleEventData.size() + vec->size() );
						vSRSSingleEventData.insert(vSRSSingleEventData.end(), vec->begin(), vec->end() );
					}
					else {
						cout<<"found NULL contents in mpd.."<<endl;
					}
				}
			}
			cout<<"Event ID: "<<entry<<endl;
			if(vSRSSingleEventData.size()!=0){entry++;}   					  // next
			cout<<"Event Size [uint_32 in number of words]:"<<vSRSSingleEventData.size()<<endl;

			if (vSRSSingleEventData.size() == 0 ) continue; // if no srs event found, go to next event
			RawDecoder raw_decoder(vSRSSingleEventData);
			//Draw raw histos
			mAPVRawHistos = raw_decoder.GetAPVRawHisto();
			map<int, map<int, TH1F*> >::iterator it;
			RawAPVs.clear();

			for(it=mAPVRawHistos.begin(); it!=mAPVRawHistos.end();++it) {

				map<int, TH1F*> temp = it->second;//
				map<int, TH1F*>::iterator itt;

				for(itt=temp.begin();itt!=temp.end();++itt) {
					TH1F* h = itt->second;
					RawAPVs.push_back(h);
				}
			}
			int nn = RawAPVs.size();
			for(int i=0; i<nn;i++) {
				cRaw->cd(i+1);
				RawAPVs[i]->SetMaximum(2000);
				RawAPVs[i]->SetMinimum(0);
				RawAPVs[i]->Draw();
			}
			cRaw->Update();
			getchar();
		}
		chan.close();
	} catch (evioException e)
    {
      cerr <<endl <<e.toString() <<endl <<endl;
      exit(EXIT_FAILURE);
    }
  return entry;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// GUI and multi-detector support
//  detector_ID, MPD   APV        strips
std::map<int,std::map<int, std::map<int, std::vector<int> > > > InputHandler::RawProcessAllEvents(int entries , map<int,map<int, map<int, std::vector<int> > > > & vRaw_Data) {

	// loading the mapping
	ifstream filestream (vDefaultMappingPath.c_str(), ifstream::in);
	printf("%s, Loading Mapping %s\n",__FUNCTION__,vDefaultMappingPath.c_str());
	string line;
	int Mapping_mpdId,Mapping_ADCId,Mapping_I2C,Mapping_GEMId,Mapping_Xis,Mapping_Pos,Mapping_Invert;
	map<int,map<int,vector<int> > > mMapping;
	if(filestream.good()){
	  while (getline(filestream, line) ) {
	    line.erase( remove_if(line.begin(), line.end(), ::isspace), line.end() );
	    if( line.find("#") == 0 ) continue;
	    char *tokens = strtok( (char *)line.data(), ",");
	    if(tokens !=NULL){
	      cout<<tokens<<" ";Mapping_mpdId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_GEMId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Xis=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_ADCId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_I2C=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Pos=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Invert=atoi(tokens);
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_GEMId);//0  detector-ID
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Xis);//1    x or y
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Pos);//2    position
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Invert);//3 invert or not
	      //cout<<"test: "<<mMapping[Mapping_mpdId][Mapping_ADCId][2]<<endl;
	    }
	    cout<<endl;
	  }
  filestream.close();
	}else{
		printf("Cannot Loading Mapping\n");
		std::map<int,std::map<int, std::map<int, std::vector<int> > > > Return_data;
		Return_data.clear();
		return Return_data;
	}
	// finnish loading the mapping

	// start loading the raw files
	std::map<int,std::map<int, std::map<int, std::vector<int> > > > Return_data;
	if(entries<=-1) {
		printf("[%s--line %d]Batch Mode Enabled, currently not support\n",__FUNCTION__, __LINE__);
	} else {
		try {
			//load the data
			evioFileChannel chan(filename.c_str(), "r");
			chan.open();
			printf("Looking for Event # %d\n", entries);
			int current_entry = 0;
			while (chan.read()) {
				map<int, map<int, map<int, vector<int> > > > mmHit;
				vSRSSingleEventData.clear();

				evioDOMTree event(chan);
				evioDOMNodeListP mpdEventList = event.getNodeList(isLeaf());
				if (current_entry == entries) { // get the target  entry, start decode the data
					evioDOMNodeList::iterator iter;
					for (iter = mpdEventList->begin();
							iter != mpdEventList->end(); ++iter) {
						if ((*iter)->tag == 10) {
							vector<uint32_t> *vec =
									(*iter)->getVector<uint32_t>();
							if (vec != NULL) {
								vSRSSingleEventData.reserve(
										vSRSSingleEventData.size()
												+ vec->size());
								vSRSSingleEventData.insert(
										vSRSSingleEventData.end(), vec->begin(),
										vec->end());
							} else {
								cout << "found NULL contents in mpd.." << endl;
							}
						}
					}
					cout << "Event ID: " << current_entry << endl;
					if (vSRSSingleEventData.size() != 0) {
						current_entry++;
						break;
					} else {
						printf("Error accrue in this events, go to next event");
						entries++;
						current_entry++;
						continue;
					}
				}
				// here did not decode the data and check the data if the current event is not the target event
				current_entry++;    //maybe this is not a good usage
			}
		} catch (evioException e) {
		}
	}

	RawDecoder raw_decoder(vSRSSingleEventData);
	std::map<int, std::map<int, std::vector<int>>> tRaw_Decoded=raw_decoder.GetDecoded();  // get the decoded data

	std::map<int, std::map<int, std::vector<int>>>::iterator iter_mpdid=tRaw_Decoded.begin();
	for(;iter_mpdid!=tRaw_Decoded.end();iter_mpdid++){
		std::map<int, std::vector<int>>::iterator itter_apvid=iter_mpdid->second.begin();
		for(;itter_apvid!=iter_mpdid->second.end();itter_apvid++) {
			Return_data[mMapping[iter_mpdid->first][itter_apvid->first][0]][iter_mpdid->first][itter_apvid->first]=itter_apvid->second;
		}
	}
	vRaw_Data=Return_data;
	return Return_data;
}

//________________________________________________
int InputHandler::PedProcessAllEvents(int entries, string pedestal_file_name)
{
	//Loading Mapping
	ifstream filestream (vDefaultMappingPath.c_str(), ifstream::in);
	printf("%s, Loading Mapping %s\n",__FUNCTION__,vDefaultMappingPath.c_str());
	string line;
	int Mapping_mpdId,Mapping_ADCId,Mapping_I2C,Mapping_GEMId,Mapping_Xis,Mapping_Pos,Mapping_Invert;
	map<int,map<int,vector<int> > > mMapping;
	if(filestream.good()){
	  while (getline(filestream, line) ) {
	    line.erase( remove_if(line.begin(), line.end(), ::isspace), line.end() );
	    if( line.find("#") == 0 ) continue;
	    char *tokens = strtok( (char *)line.data(), ",");
	    if(tokens !=NULL){
	      cout<<tokens<<" ";Mapping_mpdId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_GEMId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Xis=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_ADCId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_I2C=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Pos=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Invert=atoi(tokens);
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_GEMId);//0
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Xis);//1
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Pos);//2
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Invert);//3
	    }
	    cout<<endl;
	  }
  filestream.close();
	}else{
		printf("Cannot Loading Mapping\n");
		return -1;
	}
  int entry = 0;
  try{

    evioFileChannel chan(filename.c_str(), "r");
    chan.open();
    while(chan.read()&&entry<entries)
    {
			map<int, map<int, map<int, vector<int> > > > mmHit;
			//vector<uint32_t> srsSingleEvent;
			vSRSSingleEventData.clear();

			evioDOMTree event(chan);
			evioDOMNodeListP mpdEventList = event.getNodeList(isLeaf());

#ifndef __INFORMATION_HitProcessAllEvents_DISPLAY_FLAG__
			cout << "number of banks: " << mpdEventList->size() << endl;
#endif

			int n_bank = mpdEventList->size();
			evioDOMNodeList::iterator iter;
			for (iter = mpdEventList->begin(); iter != mpdEventList->end();
					++iter) {

				if (((*iter)->tag == 10)) {
					vector<uint32_t> *vec = (*iter)->getVector<uint32_t>();
					if (vec != NULL) {
						vSRSSingleEventData.reserve(
								vSRSSingleEventData.size() + vec->size());
						vSRSSingleEventData.insert(vSRSSingleEventData.end(),
								vec->begin(), vec->end());
					} else {
						cout << "found NULL contents in mpd.." << endl;
					}
				}
			}

#ifndef __INFORMATION_HitProcessAllEvents_DISPLAY_FLAG__
			cout << "Event ID: " << entry << endl;
#endif
			if (vSRSSingleEventData.size() != 0) {
				entry++;
			}
#ifndef __INFORMATION_HitProcessAllEvents_DISPLAY_FLAG__
			cout << "Event Size [uint_32 in number of words]:"
					<< vSRSSingleEventData.size() << endl;
#endif
			if (vSRSSingleEventData.size() == 0)
				continue; // if no srs event found, go to next event

			RawDecoder raw_decoder(vSRSSingleEventData);
			mTsAdc = raw_decoder.GetStripTsAdcMap(); //

			//Fill strip average ADC Histogram for calculating pedestal
			int mpd_id = 0;
			int adc_ch = 0;
			int stripNb = 0;
			for (map<int, map<int, map<int, vector<int> > > >::iterator it =
					mTsAdc.begin(); it != mTsAdc.end(); ++it) {
				mpd_id = it->first;
				map<int, map<int, vector<int> > > temp = it->second;

				map<int, map<int, vector<int> > >::iterator itt;
				for (itt = temp.begin(); itt != temp.end(); ++itt) {
					adc_ch = itt->first;
					map<int, vector<int> > tempp = itt->second;
					map<int, vector<int> >::iterator ittt;
					for (ittt = tempp.begin(); ittt != tempp.end(); ++ittt) {
						stripNb = ittt->first;
						vector<int> adc_temp = ittt->second;
						int adcSum_temp = 0;
						int TSsize = adc_temp.size();

						if (entry == 1)
							mPedestalHisto[mpd_id][adc_ch][stripNb] = new TH1F(
									Form("mpd_%d_ch_%d_Strip_%d", mpd_id,
											adc_ch, stripNb),
									Form("mpd_%d_ch_%d_Strip_%d_pedestal_data",
											mpd_id, adc_ch, stripNb), 3500,
									-500, 3000);

						for (int i = 0; i < TSsize; i++) {
							adcSum_temp += adc_temp[i];
						}
						adcSum_temp = adcSum_temp / TSsize;
						mPedestalHisto[mpd_id][adc_ch][stripNb]->Fill(adcSum_temp);
					}
				}
			}
      //end of preparation for calculating pedestal
    }
    chan.close();
  } catch (evioException e) {
    cerr <<endl <<e.toString() <<endl <<endl;
    exit(EXIT_FAILURE);
  }

  //
  int mpd_id=0;
  int adc_ch=0;
  int stripNb=0;
  //
  //Calculating pedestal
  char *PedFilename_temp = new char[100];
  std::strcpy(PedFilename_temp,pedestal_file_name.c_str());

	f = new TFile(PedFilename_temp, "RECREATE");
	delete[] PedFilename_temp;


	std::map<int,TH1F*> mPedestalRMSall;
	std::map<int,TH1F*> mPedestalMeanall;
	std::map<int,TH1F*> mPedestalMeanx;
	std::map<int,TH1F*> mPedestalMeany;
	std::map<int,TH1F*> mPedestalRMSx;
	std::map<int,TH1F*> mPedestalRMSy;

	std::vector<int> vChamberID;
	// check the number of Detector attached
	for(std::map<int, std::map<int, std::map<int, TH1F*> > >::iterator iter_mpd=mPedestalHisto.begin(); iter_mpd!=mPedestalHisto.end();iter_mpd++){
		for(std::map<int, std::map<int, TH1F*> >::iterator itter_apv=iter_mpd->second.begin(); itter_apv!=iter_mpd->second.end();itter_apv++){
			vChamberID.push_back(mMapping[iter_mpd->first][itter_apv->first][0]);
		}
	}
	sort(vChamberID.begin(),vChamberID.end());
	vChamberID.erase(unique(vChamberID.begin(),vChamberID.end()),vChamberID.end());
	// find the chamberID, initilize the histo
	for(unsigned int i=0; i < vChamberID.size();i++){
		mPedestalRMSall[vChamberID[i]]= new TH1F(Form("Chamber 39 APV Pedestal RMS Distribution",vChamberID[i]),Form("Chamber 39 APV Pedestal RMS Distribution",vChamberID[i]),100,0,100);
		mPedestalRMSall[vChamberID[i]]->GetXaxis()->SetTitle("ADC channels");
		mPedestalRMSall[vChamberID[i]]->GetYaxis()->SetTitle("# of readout channels");

		mPedestalMeanall[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal Mean Distribution",vChamberID[i]),Form("Chamber %d APV Pedestal Mean Distribution",vChamberID[i]),200,0,200);
		mPedestalMeanall[vChamberID[i]]->GetXaxis()->SetTitle("ADC channels");
		mPedestalMeanall[vChamberID[i]]->GetYaxis()->SetTitle("# of readout channels");

		mPedestalMeanx[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal Mean X",vChamberID[i]),Form("Chamber %d APV Pedestal Mean X",vChamberID[i]),1600,0,1600);
		mPedestalMeanx[vChamberID[i]]->GetXaxis()->SetTitle("position");
		mPedestalMeanx[vChamberID[i]]->GetYaxis()->SetTitle("ADC channels");


		mPedestalMeany[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal Mean Y",vChamberID[i]),Form("Chamber %d APV Pedestal Mean Y",vChamberID[i]),1280,0,1280);
		mPedestalMeany[vChamberID[i]]->GetXaxis()->SetTitle("position");
		mPedestalMeany[vChamberID[i]]->GetYaxis()->SetTitle("ADC channels");

		mPedestalRMSx[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal RMS X",vChamberID[i]),Form("Chamber %d APV Pedestal RMS X",vChamberID[i]),1600,0,1600);
		mPedestalRMSx[vChamberID[i]]->GetXaxis()->SetTitle("position");
		mPedestalRMSx[vChamberID[i]]->GetYaxis()->SetTitle("ADC channels");


		mPedestalRMSy[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal RMS Y",vChamberID[i]),Form("Chamber %d APV Pedestal RMS Y",vChamberID[i]),1280,0,1280);
		mPedestalRMSy[vChamberID[i]]->GetXaxis()->SetTitle("position");
		mPedestalRMSy[vChamberID[i]]->GetYaxis()->SetTitle("ADC channels");
	}

	// save the files
	map<int, map<int, map<int, TH1F*> > >::iterator it;
	for (it = mPedestalHisto.begin(); it != mPedestalHisto.end(); ++it) {   // MPD loop
		mpd_id = it->first;
		map<int, map<int, TH1F*> > temp = it->second;

		map<int, map<int, TH1F*> >::iterator itt;
		for (itt = temp.begin(); itt != temp.end(); ++itt) {     // ADC(apv)loop
			adc_ch = itt->first;
			map<int, TH1F*> tempp = itt->second;
			map<int, TH1F*>::iterator ittt;
			mPedestalMean[mpd_id][adc_ch] = new TH1F(Form("PedestalMean(offset)_mpd_%d_ch_%d", mpd_id, adc_ch),Form("PedestalMean(offset)_mpd_%d_ch_%d", mpd_id, adc_ch), 128, 0, 128);
			mPedestalRMS[mpd_id][adc_ch] = new TH1F(Form("PedestalRMS_mpd_%d_ch_%d", mpd_id, adc_ch),Form("PedestalRMS_mpd_%d_ch_%d", mpd_id, adc_ch),128, 0, 128);
			// add the overall RMS and mean histo
			for (ittt = tempp.begin(); ittt != tempp.end(); ++ittt) {     // strips loop
				stripNb = ittt->first;
				TH1F* Pedestal_temp = ittt->second;
				float mean = Pedestal_temp->GetMean(); //cout<<"Mean:  "<<mean<<endl;
				float rms = Pedestal_temp->GetRMS();

				int RstripNb = 32 * (stripNb % 4) + 8 * (int) (stripNb / 4)
						- 31 * (int) (stripNb / 16); //channel re-matching for apv25 chip

						//stripNb=(8*(int)(stripNb/4)+3-stripNb)*((int)(stripNb/4)%2)+stripNb*(1-((int)(stripNb/4)%2));
				RstripNb = RstripNb + 1 + RstripNb % 4
						- 5 * (((int) (RstripNb / 4)) % 2); //channel re-matching for INFN type APV front-end card

				//invert the address if needed
				RstripNb=RstripNb+(127-2*RstripNb)*mMapping[mpd_id][adc_ch][3];//re-matching for inverted strips Nb
				int RstripPos=RstripNb+128*mMapping[mpd_id][adc_ch][2];            // calculate position

				if(mMapping[mpd_id][adc_ch][1]==0){
					mPedestalMeanx[mMapping[mpd_id][adc_ch][0]]->Fill(RstripPos,mean);
					mPedestalRMSx[mMapping[mpd_id][adc_ch][0]]->Fill(RstripPos,rms);
				}
				if(mMapping[mpd_id][adc_ch][1]==1){
					mPedestalMeany[mMapping[mpd_id][adc_ch][0]]->Fill(RstripPos,mean);
					mPedestalRMSy[mMapping[mpd_id][adc_ch][0]]->Fill(RstripPos,rms);
				}

				mPedestalMean[mpd_id][adc_ch]->Fill(stripNb, mean);
				mPedestalRMS[mpd_id][adc_ch]->Fill(stripNb, rms);

				mPedestalMeanall[mMapping[mpd_id][adc_ch][0]]->Fill(mean);
				mPedestalRMSall[mMapping[mpd_id][adc_ch][0]]->Fill(rms);
			}
		}
	}
	f->Write();
	f->Close();

  //delete histograms
  for(map<int, map<int, map<int,vector<int> > > >::iterator it = mTsAdc.begin(); it!=mTsAdc.end(); ++it)
    {
      mpd_id = it->first;
      map<int, map<int,vector<int> > > temp = it->second;

      map<int, map<int,vector<int> > >::iterator itt;
      for(itt=temp.begin(); itt!=temp.end(); ++itt)
	{
	  adc_ch = itt->first;
	  map<int,vector<int> > tempp = itt->second;
	  map<int,vector<int> >::iterator ittt;
	  for(ittt=tempp.begin(); ittt!=tempp.end(); ++ittt)
	    {
	      stripNb = ittt->first;
	      mPedestalHisto[mpd_id][adc_ch][stripNb]->Delete();
	    }
	}
    }
  return entry;
}



int InputHandler::PedProcessAllEvents(string pedestal_file_name ) {

	//Loading Mapping
	ifstream filestream (vDefaultMappingPath.c_str(), ifstream::in);

	string line;
	int Mapping_mpdId,Mapping_ADCId,Mapping_I2C,Mapping_GEMId,Mapping_Xis,Mapping_Pos,Mapping_Invert;
	map<int,map<int,vector<int> > > mMapping;
	if(filestream.good()){
	while (getline(filestream, line) ) {
	    line.erase( remove_if(line.begin(), line.end(), ::isspace), line.end() );
	    if( line.find("#") == 0 ) continue;
	    char *tokens = strtok( (char *)line.data(), ",");
	    if(tokens !=NULL){
	      cout<<tokens<<" ";Mapping_mpdId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_GEMId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Xis=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_ADCId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_I2C=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Pos=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Invert=atoi(tokens);
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_GEMId);//0
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Xis);//1
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Pos);//2
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Invert);//3
	    }
	    cout<<endl;
	  }
  filestream.close();
	}else{
		printf("Can not loading mapping file\n");
		return -1;
	}

  int entry = 0;
	try {

		evioFileChannel chan(filename.c_str(), "r");

		chan.open();

		while (chan.read()) {
			map<int, map<int, map<int, vector<int> > > > mmHit;
			//vector<uint32_t> srsSingleEvent;
			vSRSSingleEventData.clear();

			evioDOMTree event(chan);
			evioDOMNodeListP mpdEventList = event.getNodeList(isLeaf());

#ifndef __INFORMATION_HitProcessAllEvents_DISPLAY_FLAG__
			cout << "number of banks: " << mpdEventList->size() << endl;
#endif

			int n_bank = mpdEventList->size();
			//if(n_bank)

			evioDOMNodeList::iterator iter;
			for (iter = mpdEventList->begin(); iter != mpdEventList->end();
					++iter) {

				if (((*iter)->tag == 10)) {
					vector<uint32_t> *vec = (*iter)->getVector<uint32_t>();
					if (vec != NULL) {
						vSRSSingleEventData.reserve(
								vSRSSingleEventData.size() + vec->size());
						vSRSSingleEventData.insert(vSRSSingleEventData.end(),
								vec->begin(), vec->end());
					} else {
						cout << "found NULL contents in mpd.." << endl;
					}
				}
			}

#ifndef __INFORMATION_HitProcessAllEvents_DISPLAY_FLAG__
			cout << "Event ID: " << entry << endl;
#endif
			if (vSRSSingleEventData.size() != 0) {
				entry++;
			}
#ifndef __INFORMATION_HitProcessAllEvents_DISPLAY_FLAG__
			cout << "Event Size [uint_32 in number of words]:"
					<< vSRSSingleEventData.size() << endl;
#endif

			if (vSRSSingleEventData.size() == 0)
				continue; // if no srs event found, go to next event

			RawDecoder raw_decoder(vSRSSingleEventData);
			mTsAdc = raw_decoder.GetStripTsAdcMap(); //

			//Fill strip average ADC Histogram for calculating pedestal
			int mpd_id = 0;
			int adc_ch = 0;
			int stripNb = 0;
			for (map<int, map<int, map<int, vector<int> > > >::iterator it =
					mTsAdc.begin(); it != mTsAdc.end(); ++it) {
				mpd_id = it->first;
				map<int, map<int, vector<int> > > temp = it->second;

				map<int, map<int, vector<int> > >::iterator itt;
				for (itt = temp.begin(); itt != temp.end(); ++itt) {
					adc_ch = itt->first;
					map<int, vector<int> > tempp = itt->second;
					map<int, vector<int> >::iterator ittt;
					for (ittt = tempp.begin(); ittt != tempp.end(); ++ittt) {
						stripNb = ittt->first;
						vector<int> adc_temp = ittt->second;
						int adcSum_temp = 0;
						int TSsize = adc_temp.size();

						if (entry == 1)
							mPedestalHisto[mpd_id][adc_ch][stripNb] = new TH1F(
									Form("mpd_%d_ch_%d_Strip_%d", mpd_id,
											adc_ch, stripNb),
									Form("mpd_%d_ch_%d_Strip_%d_pedestal_data",
											mpd_id, adc_ch, stripNb), 3500,
									-500, 3000);

						for (int i = 0; i < TSsize; i++) {
							adcSum_temp += adc_temp[i];
							//mPedestalHisto[mpd_id][adc_ch][stripNb]->Fill(adc_temp[i]);
							//cout<<"mean: "<<adc_temp[i]<<endl;
						}
						adcSum_temp = adcSum_temp / TSsize;
						mPedestalHisto[mpd_id][adc_ch][stripNb]->Fill(
								adcSum_temp);

					}
				}
			}
			//end of preparation for calculating pedestal
		}
		chan.close();
	} catch (evioException e) {
		cerr << endl << e.toString() << endl << endl;
		exit(EXIT_FAILURE);
	}

  //
  int mpd_id=0;
  int adc_ch=0;
  int stripNb=0;
  //
  //Calculating pedestal
  char *PedFilename_temp = new char[100];
  std::strcpy(PedFilename_temp,pedestal_file_name.c_str());

	f = new TFile(PedFilename_temp, "RECREATE");
	delete[] PedFilename_temp;


	std::map<int,TH1F*> mPedestalRMSall;
	std::map<int,TH1F*> mPedestalMeanall;
	std::map<int,TH1F*> mPedestalMeanx;
	std::map<int,TH1F*> mPedestalMeany;
	std::map<int,TH1F*> mPedestalRMSx;
	std::map<int,TH1F*> mPedestalRMSy;

	std::vector<int> vChamberID;
	// check the number of Detector attached
	for(std::map<int, std::map<int, std::map<int, TH1F*> > >::iterator iter_mpd=mPedestalHisto.begin(); iter_mpd!=mPedestalHisto.end();iter_mpd++){
		for(std::map<int, std::map<int, TH1F*> >::iterator itter_apv=iter_mpd->second.begin(); itter_apv!=iter_mpd->second.end();itter_apv++){
			if((mMapping.find(iter_mpd->first)!=mMapping.end())&&(mMapping[iter_mpd->first].find(itter_apv->first)!=mMapping[iter_mpd->first].end())) {
			vChamberID.push_back(mMapping[iter_mpd->first][itter_apv->first][0]);}
		}
	}
	sort(vChamberID.begin(),vChamberID.end());
	vChamberID.erase(unique(vChamberID.begin(),vChamberID.end()),vChamberID.end());
	// find the chamberID, initilize the histo
	for(unsigned int i=0; i < vChamberID.size();i++){
		mPedestalRMSall[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal RMS Distribution",vChamberID[i]),Form("Chamber %d APV Pedestal RMS Distribution",vChamberID[i]),100,0,100);
		mPedestalRMSall[vChamberID[i]]->GetXaxis()->SetTitle("ADC channels");
		mPedestalRMSall[vChamberID[i]]->GetYaxis()->SetTitle("# of readout channels");

		mPedestalMeanall[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal Mean Distribution",vChamberID[i]),Form("Chamber %d APV Pedestal Mean Distribution",vChamberID[i]),200,0,200);
		mPedestalMeanall[vChamberID[i]]->GetXaxis()->SetTitle("ADC channels");
		mPedestalMeanall[vChamberID[i]]->GetYaxis()->SetTitle("# of readout channels");

		mPedestalMeanx[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal Mean X",vChamberID[i]),Form("Chamber %d APV Pedestal Mean X",vChamberID[i]),1600,0,1600);
		mPedestalMeanx[vChamberID[i]]->GetXaxis()->SetTitle("position");
		mPedestalMeanx[vChamberID[i]]->GetYaxis()->SetTitle("ADC channels");


		mPedestalMeany[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal Mean Y",vChamberID[i]),Form("Chamber %d APV Pedestal Mean Y",vChamberID[i]),1280,0,1280);
		mPedestalMeany[vChamberID[i]]->GetXaxis()->SetTitle("position");
		mPedestalMeany[vChamberID[i]]->GetYaxis()->SetTitle("ADC channels");

		mPedestalRMSx[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal RMS X",vChamberID[i]),Form("Chamber %d APV Pedestal RMS X",vChamberID[i]),1600,0,1600);
		mPedestalRMSx[vChamberID[i]]->GetXaxis()->SetTitle("position");
		mPedestalRMSx[vChamberID[i]]->GetYaxis()->SetTitle("ADC channels");


		mPedestalRMSy[vChamberID[i]]= new TH1F(Form("Chamber %d APV Pedestal RMS Y",vChamberID[i]),Form("Chamber %d APV Pedestal RMS Y",vChamberID[i]),1280,0,1280);
		mPedestalRMSy[vChamberID[i]]->GetXaxis()->SetTitle("position");
		mPedestalRMSy[vChamberID[i]]->GetYaxis()->SetTitle("ADC channels");
	}

	// save the files
	map<int, map<int, map<int, TH1F*> > >::iterator it;
	for (it = mPedestalHisto.begin(); it != mPedestalHisto.end(); ++it) {   // MPD loop
		mpd_id = it->first;
		map<int, map<int, TH1F*> > temp = it->second;

		map<int, map<int, TH1F*> >::iterator itt;
		for (itt = temp.begin(); itt != temp.end(); ++itt) {     // ADC(apv)loop
			adc_ch = itt->first;
			if((mMapping.find(mpd_id)!=mMapping.end())&&(mMapping[mpd_id].find(adc_ch)!=mMapping[mpd_id].end())) {
			map<int, TH1F*> tempp = itt->second;
			map<int, TH1F*>::iterator ittt;
			mPedestalMean[mpd_id][adc_ch] = new TH1F(Form("PedestalMean(offset)_mpd_%d_ch_%d", mpd_id, adc_ch),Form("PedestalMean(offset)_mpd_%d_ch_%d", mpd_id, adc_ch), 128, 0, 128);
			mPedestalRMS[mpd_id][adc_ch] = new TH1F(Form("PedestalRMS_mpd_%d_ch_%d", mpd_id, adc_ch),Form("PedestalRMS_mpd_%d_ch_%d", mpd_id, adc_ch),128, 0, 128);
			// add the overall RMS and mean histo
			for (ittt = tempp.begin(); ittt != tempp.end(); ++ittt) {     // strips loop
				stripNb = ittt->first;
				TH1F* Pedestal_temp = ittt->second;
				float mean = Pedestal_temp->GetMean(); //cout<<"Mean:  "<<mean<<endl;
				float rms = Pedestal_temp->GetRMS();

				int RstripNb = 32 * (stripNb % 4) + 8 * (int) (stripNb / 4)
						- 31 * (int) (stripNb / 16); //channel re-matching for apv25 chip

						//stripNb=(8*(int)(stripNb/4)+3-stripNb)*((int)(stripNb/4)%2)+stripNb*(1-((int)(stripNb/4)%2));
				RstripNb = RstripNb + 1 + RstripNb % 4
						- 5 * (((int) (RstripNb / 4)) % 2); //channel re-matching for INFN type APV front-end card

				//invert the address if needed
				RstripNb=RstripNb+(127-2*RstripNb)*mMapping[mpd_id][adc_ch][3];//re-matching for inverted strips Nb
				int RstripPos=RstripNb+128*mMapping[mpd_id][adc_ch][2];            // calculate position

				if(mMapping[mpd_id][adc_ch][1]==0){
					mPedestalMeanx[mMapping[mpd_id][adc_ch][0]]->Fill(RstripPos,mean);
					mPedestalRMSx[mMapping[mpd_id][adc_ch][0]]->Fill(RstripPos,rms);
				}
				if(mMapping[mpd_id][adc_ch][1]==1){
					mPedestalMeany[mMapping[mpd_id][adc_ch][0]]->Fill(RstripPos,mean);
					mPedestalRMSy[mMapping[mpd_id][adc_ch][0]]->Fill(RstripPos,rms);
				}

				mPedestalMean[mpd_id][adc_ch]->Fill(stripNb, mean);
				mPedestalRMS[mpd_id][adc_ch]->Fill(stripNb, rms);

				mPedestalMeanall[mMapping[mpd_id][adc_ch][0]]->Fill(mean);
				mPedestalRMSall[mMapping[mpd_id][adc_ch][0]]->Fill(rms);
				}
			}
		}
	}
	f->Write();
	f->Close();

  //delete histograms
  for(map<int, map<int, map<int,vector<int> > > >::iterator it = mTsAdc.begin(); it!=mTsAdc.end(); ++it)
    {
      mpd_id = it->first;
      map<int, map<int,vector<int> > > temp = it->second;

      map<int, map<int,vector<int> > >::iterator itt;
      for(itt=temp.begin(); itt!=temp.end(); ++itt)
	{
	  adc_ch = itt->first;
	  map<int,vector<int> > tempp = itt->second;
	  map<int,vector<int> >::iterator ittt;
	  for(ittt=tempp.begin(); ittt!=tempp.end(); ++ittt)
	    {
	      stripNb = ittt->first;
	      mPedestalHisto[mpd_id][adc_ch][stripNb]->Delete();
	    }
	}
    }
  return entry;

}


map<int,map<int,int>> InputHandler::ZeroSProcessAllEvents(int entries, string gui, string pedestal_file_name){

	map<int,map<int,int>> ZeroSReturn;  // 1 x before zero subression 2 x afterzerosubration, 3 after remove cross talk  4 cross talk
										// 11       12                13
	//Loading Mapping
	ifstream filestream (vDefaultMappingPath.c_str(), ifstream::in);
	string line;
	int Mapping_mpdId,Mapping_ADCId,Mapping_I2C,Mapping_GEMId,Mapping_Xis,Mapping_Pos,Mapping_Invert;
	map<int,map<int,vector<int> > > mMapping;
if(filestream.good()){
	while (getline(filestream, line) ) {
	    line.erase( remove_if(line.begin(), line.end(), ::isspace), line.end() );
	    if( line.find("#") == 0 ) continue;
	    char *tokens = strtok( (char *)line.data(), ",");
	    if(tokens !=NULL){
	      cout<<tokens<<" ";Mapping_mpdId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_GEMId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Xis=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_ADCId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_I2C=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Pos=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Invert=atoi(tokens);
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_GEMId);//0
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Xis);//1
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Pos);//2
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Invert);//3
	      //cout<<"test: "<<mMapping[Mapping_mpdId][Mapping_ADCId][2]<<endl;
	    }
	    cout<<endl;
	  }
	  filestream.close();
}else{
	printf("Cannot loading Mapping\n");
	return ZeroSReturn;
}
	  //
	  char *PedFilename_temp = new char[100];
	 	  std::strcpy(PedFilename_temp,pedestal_file_name.c_str());    // load the pedestal file
	 	  f = new TFile(PedFilename_temp,"READ" );
	 	  delete[] PedFilename_temp;


	  try {
			//  load the data
			evioFileChannel chan(filename.c_str(), "r");
			chan.open();
			printf("Looking for Event # %d\n", entries);
			int current_entry = 0;
			while (chan.read()) {
				map<int, map<int, map<int, vector<int> > > > mmHit;
				vSRSSingleEventData.clear();

				evioDOMTree event(chan);
				evioDOMNodeListP mpdEventList = event.getNodeList(isLeaf());
				if (current_entry == entries) { // get the target  entry, start decode the data
					evioDOMNodeList::iterator iter;
					for (iter = mpdEventList->begin(); iter != mpdEventList->end();
							++iter) {
						if ((*iter)->tag == 10) {
							vector<uint32_t> *vec = (*iter)->getVector<uint32_t>();
							if (vec != NULL) {
								vSRSSingleEventData.reserve(
										vSRSSingleEventData.size() + vec->size());
								vSRSSingleEventData.insert(
										vSRSSingleEventData.end(), vec->begin(),
										vec->end());
							} else {
								cout << "found NULL contents in mpd.." << endl;
							}
						}
					}
					cout << "Event ID: " << current_entry << endl;
					if (vSRSSingleEventData.size() != 0) {
						current_entry++;
						break;
					} else {
						printf("Error accrue in this events, go to next event");
						entries++;
						current_entry++;
						continue;
					}
				}
				// here did not decode the data and check the data if the current event is not the target event
				current_entry++;    //maybe this is not a good usage
			}
		} catch (evioException e) {
			cerr << endl << e.toString() << endl << endl;
			exit(EXIT_FAILURE);
		}

		  RawDecoder raw_decoder(vSRSSingleEventData);
		  //map<mpd_id, map<ch_id, vector<int> > >, vector: adc values (all time samples)
		  mTsAdc = raw_decoder.GetStripTsAdcMap();//

		  int mpd_id=0;
		  int adc_ch=0;
		  int stripNb=0;
		  //MPD_ID,    ADC_channels stripsNB   six timesample
	      for(map<int, map<int, map<int,vector<int> > > >::iterator it = mTsAdc.begin(); it!=mTsAdc.end(); ++it) {
	    	  mpd_id=it->first;
	    	  map<int, map<int, vector<int> > > temp=it->second;

	    	  map<int, map<int,vector<int> > >::iterator itt;
	    	  for(itt=temp.begin(); itt!=temp.end(); ++itt) {
	    	      adc_ch = itt->first;
	    	      map<int,vector<int> > tempp = itt->second;
	    	      map<int,vector<int> >::iterator ittt;

	    	      //loading pedestal information from root files
	    	      TH1F* hMean = (TH1F*)f->Get(Form("PedestalMean(offset)_mpd_%d_ch_%d",mpd_id, adc_ch));  // read the pedestal
	    	      TH1F* hRMS  = (TH1F*)f->Get(Form("PedestalRMS_mpd_%d_ch_%d",mpd_id, adc_ch));			  // read the pedestal
	    	      // loop an all the Strips
	    	      // added by Siyu, To be improve
	    	      map<int,int> sEventCrossTalk_temp;
	    	      map<int,int> AddressCorrelation;
	    	      //printf("lalallalsals\n\n\n");
	    	      for(ittt=tempp.begin();ittt!=tempp.end();++ittt) {
	    			  stripNb = ittt->first;

	    			  vector<int> adc_temp = ittt->second;   // buffer the six(or other) Timesample ADC value
	    			  int adcSum_temp=0;
	    			  int TSsize = adc_temp.size();
	    			  for(int i=0; i<TSsize;i++)
	    			    { adcSum_temp+=adc_temp[i];
	    			    }
	    			  adcSum_temp=adcSum_temp/TSsize;  // get the mean of the six time sample

	    			  // convert the address to the real strips address
	    			  int RstripNb=32*(stripNb%4)+8*(int)(stripNb/4)-31*(int)(stripNb/16);//channel re-matching for apv25 chip
	    			  int RstripPos;

	    			  //stripNb=(8*(int)(stripNb/4)+3-stripNb)*((int)(stripNb/4)%2)+stripNb*(1-((int)(stripNb/4)%2));
	    			  RstripNb=RstripNb+1+RstripNb%4-5*(((int)(RstripNb/4))%2);//channel re-matching for INFN type APV front-end card, ???????
	    			  //invert the address if needed
	    			  RstripNb=RstripNb+(127-2*RstripNb)*mMapping[mpd_id][adc_ch][3];//re-matching for inverted strips Nb
	    			  RstripPos=RstripNb+128*mMapping[mpd_id][adc_ch][2];            // calculate position

	    			  float fadcvalue_temp = adcSum_temp-hMean->GetBinContent(stripNb+1);   // zero subtraction
	    			  float rms_temp=hRMS->GetBinContent(stripNb+1);
	    			  if(mMapping[mpd_id][adc_ch][1]==0){
	    				  ZeroSReturn[1][RstripPos]=fadcvalue_temp;
	    				  //hhX->Fill(RstripPos,fadcvalue_temp);
	    			  }
	    			  if(mMapping[mpd_id][adc_ch][1]==1){
	    				  //hhY->Fill(RstripPos,fadcvalue_temp);
	    				  ZeroSReturn[11][RstripPos]=fadcvalue_temp;
	    			  }

	    			  // apply 5 sigma select the effective hit
	    			  if(( fadcvalue_temp>ZERO_CONPRESION_SIGMA_THR*rms_temp))
	    			    {
	    			      //int detID=mMapping[mpd_id][adc_ch][0];
	    			      //int planeID=mMapping[mpd_id][adc_ch][1];
	    			      if(mMapping[mpd_id][adc_ch][1]==0){
	    			    	  //hX->Fill(RstripPos,fadcvalue_temp);
	    			    	  ZeroSReturn[2][RstripPos]=fadcvalue_temp;

	    			      }
	    			      if(mMapping[mpd_id][adc_ch][1]==1){
	    			    	  //hY->Fill(RstripPos,fadcvalue_temp);
	    			    	  ZeroSReturn[12][RstripPos]=fadcvalue_temp;
	    			      }
	    			      cout<<"orginal strip: "<<stripNb<<" hit in MPD:"<<mpd_id<<" adc:"<<adc_ch<<" strip: "<<RstripNb+1<<"Position: "<<RstripPos<<" adc value:"<<fadcvalue_temp<<" RMS:"<<rms_temp<<endl;
	    			      sEventCrossTalk_temp[stripNb]=fadcvalue_temp;
	    			      AddressCorrelation[stripNb]=RstripPos;
	    			    }
	    			  //hhh->Delete();
	    	      }
	    	      map<int,int> sRemoveCrossTalk;
	    	      map<int,int> sCrossTalk;
	    	      FindCrossTalk(AddressCorrelation,sEventCrossTalk_temp,&sRemoveCrossTalk,&sCrossTalk);
	    	      //cout<<" crosstalk size="<<sRemoveCrossTalk.size()<<" crosstalk size="<<sCrossTalk.size()<<endl;
	    	      map<int,int>::iterator iter_rmcttemp=sRemoveCrossTalk.begin();
	    	      while(iter_rmcttemp!=sRemoveCrossTalk.end()){
	    	    	  if(mMapping[mpd_id][adc_ch][1]==0){
	    	    		  ZeroSReturn[3][iter_rmcttemp->first]=iter_rmcttemp->second;
	    	    		  //hX_rmCt->Fill(iter_rmcttemp->first,iter_rmcttemp->second);
	    	    	  }
	    	    	  if(mMapping[mpd_id][adc_ch][1]==1){
	    	    		  ZeroSReturn[13][iter_rmcttemp->first]=iter_rmcttemp->second;
	    	    		  //hY_rmCt->Fill(iter_rmcttemp->first,iter_rmcttemp->second);
	    	    	  }
	    	    	  iter_rmcttemp++;
	    	      }
	    	      map<int,int>::iterator iter_cttemp=sCrossTalk.begin();
	    	      while(iter_cttemp!= sCrossTalk.end()) {
	    	    	  if(mMapping[mpd_id][adc_ch][1]==0){
	    	    		  ZeroSReturn[4][iter_cttemp->first]=iter_cttemp->second;
	    	    		  //hX_Ct->Fill(iter_cttemp->first,iter_cttemp->second);
	    	    	  }
	    	    	  if(mMapping[mpd_id][adc_ch][1]==1){
	    	    		  ZeroSReturn[14][iter_cttemp->first]=iter_cttemp->second;
	    	    		  //hY_Ct->Fill(iter_cttemp->first,iter_cttemp->second);
	    	    	  }
	    	    	  iter_cttemp++;
	    	      }
	    	      hMean -> Delete();
	    	      hRMS  -> Delete();
	    	  }

	      }

return ZeroSReturn;
}

// used for multi-detector mode
// detectorID     x or Y     strips  ADC
std::map<int,std::map<int,std::map<int,int> > > InputHandler::ZeroSProcessSingleEvents(int entries, std::map<int,std::map<int,std::map<int,int> > > & vRaw_Pedesta, string pedestal_file_name,int SampleID)  {

	// 1 x before zero subression 2 x afterzerosubration, 3 after remove cross talk  4 cross talk
	// 11       12                13
	std::map<int,std::map<int,std::map<int,int> > > vRaw_Pedestal_return;   // used for buffer the temp data

	//Loading Mapping
	ifstream filestream (vDefaultMappingPath.c_str(), ifstream::in);
	string line;
	int Mapping_mpdId,Mapping_ADCId,Mapping_I2C,Mapping_GEMId,Mapping_Xis,Mapping_Pos,Mapping_Invert;
	map<int,map<int,vector<int> > > mMapping;
	if(filestream.good()){
		while (getline(filestream, line) ) {
			line.erase( remove_if(line.begin(), line.end(), ::isspace), line.end() );
			if( line.find("#") == 0 ) continue;
			char *tokens = strtok( (char *)line.data(), ",");
			if(tokens !=NULL){
				cout<<tokens<<" ";Mapping_mpdId=atoi(tokens);
				tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_GEMId=atoi(tokens);
				tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Xis=atoi(tokens);
				tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_ADCId=atoi(tokens);
				tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_I2C=atoi(tokens);
				tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Pos=atoi(tokens);
				tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Invert=atoi(tokens);
				mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_GEMId);//0
				mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Xis);//1
				mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Pos);//2
				mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Invert);//3
				//cout<<"test: "<<mMapping[Mapping_mpdId][Mapping_ADCId][2]<<endl;
			}
			cout << endl;
		}
		filestream.close();
	} else {
		printf("Cannot loading Mapping\n");
		return vRaw_Pedestal_return;
	}
	//
	char *PedFilename_temp = new char[100];
	std::strcpy(PedFilename_temp, pedestal_file_name.c_str()); // load the pedestal file
	f = new TFile(PedFilename_temp, "READ");
	delete[] PedFilename_temp;

	try {
			//  load the data
			evioFileChannel chan(filename.c_str(), "r");
			chan.open();
			printf("Looking for Event # %d\n", entries);
			int current_entry = 0;
			while (chan.read()) {
				map<int, map<int, map<int, vector<int> > > > mmHit;
				vSRSSingleEventData.clear();
				evioDOMTree event(chan);
				evioDOMNodeListP mpdEventList = event.getNodeList(isLeaf());
				if (current_entry == entries) { // get the target  entry, start decode the data
					evioDOMNodeList::iterator iter;
					for (iter = mpdEventList->begin(); iter != mpdEventList->end();
							++iter) {
						if ((*iter)->tag == 10) {
							vector<uint32_t> *vec = (*iter)->getVector<uint32_t>();
							if (vec != NULL) {
								vSRSSingleEventData.reserve(
										vSRSSingleEventData.size() + vec->size());
								vSRSSingleEventData.insert(
										vSRSSingleEventData.end(), vec->begin(),
										vec->end());
							} else {
								cout << "found NULL contents in mpd.." << endl;
							}
						}
					}
					cout << "Event ID: " << current_entry << endl;
					if (vSRSSingleEventData.size() != 0) {
						current_entry++;
						break;
					} else {
						printf("Error accrue in this events, go to next event");
						entries++;
						current_entry++;
						continue;
					}
				}
				// here did not decode the data and check the data if the current event is not the target event
				current_entry++;    //maybe this is not a good usage
			}
		} catch (evioException e) {
			cerr << endl << e.toString() << endl << endl;
			exit(EXIT_FAILURE);
		}

	RawDecoder raw_decoder(vSRSSingleEventData);
	//map<mpd_id, map<ch_id, vector<int> > >, vector: adc values (all time samples)
	mTsAdc = raw_decoder.GetStripTsAdcMap();    //

	  int mpd_id=0;
	  int adc_ch=0;
	  int stripNb=0;
	  //MPD_ID,    ADC_channels stripsNB   six timesample
    for(map<int, map<int, map<int,vector<int> > > >::iterator it = mTsAdc.begin(); it!=mTsAdc.end(); ++it) {
  	  mpd_id=it->first;
  	  map<int, map<int, vector<int> > > temp=it->second;

  	  map<int, map<int,vector<int> > >::iterator itt;
  	  for(itt=temp.begin(); itt!=temp.end(); ++itt) {
  	      adc_ch = itt->first;
  	      map<int,vector<int> > tempp = itt->second;
  	      map<int,vector<int> >::iterator ittt;

  	      //loading pedestal information from root files
  	      TH1F* hMean = (TH1F*)f->Get(Form("PedestalMean(offset)_mpd_%d_ch_%d",mpd_id, adc_ch));  // read the pedestal
  	      TH1F* hRMS  = (TH1F*)f->Get(Form("PedestalRMS_mpd_%d_ch_%d",mpd_id, adc_ch));			  // read the pedestal
  	      // loop an all the Strips
  	      // added by Siyu, To be improve
  	      map<int,int> sEventCrossTalk_temp;
  	      map<int,int> AddressCorrelation;
  	      //printf("lalallalsals\n\n\n");
  	      for(ittt=tempp.begin();ittt!=tempp.end();++ittt) {
  			  stripNb = ittt->first;

  			  vector<int> adc_temp = ittt->second;   // buffer the six(or other) Timesample ADC value
  			  int adcSum_temp=0;
  			  int TSsize = adc_temp.size();
  			  for(int i=0; i<TSsize;i++)
  			    { adcSum_temp+=adc_temp[i];
  			    }
  			  adcSum_temp=adcSum_temp/TSsize;  // get the mean of the six time sample

  			  // convert the address to the real strips address
  			  int RstripNb=32*(stripNb%4)+8*(int)(stripNb/4)-31*(int)(stripNb/16);//channel re-matching for apv25 chip
  			  int RstripPos;

  			  //stripNb=(8*(int)(stripNb/4)+3-stripNb)*((int)(stripNb/4)%2)+stripNb*(1-((int)(stripNb/4)%2));
  			  RstripNb=RstripNb+1+RstripNb%4-5*(((int)(RstripNb/4))%2);//channel re-matching for INFN type APV front-end card, ???????
  			  //invert the address if needed
  			  RstripNb=RstripNb+(127-2*RstripNb)*mMapping[mpd_id][adc_ch][3];//re-matching for inverted strips Nb
  			  RstripPos=RstripNb+128*mMapping[mpd_id][adc_ch][2];            // calculate position

  			  float fadcvalue_temp = adcSum_temp-hMean->GetBinContent(stripNb+1);   // zero subtraction
  			  float rms_temp=hRMS->GetBinContent(stripNb+1);
  			  if(mMapping[mpd_id][adc_ch][1]==0){
  				vRaw_Pedestal_return[mMapping[mpd_id][adc_ch][0]][1][RstripPos]=fadcvalue_temp;
  			  }
  			  if(mMapping[mpd_id][adc_ch][1]==1){
  				vRaw_Pedestal_return[mMapping[mpd_id][adc_ch][0]][11][RstripPos]=fadcvalue_temp;
  			  }

  			  // apply 5 sigma select the effective hit
  			  if(( fadcvalue_temp>ZERO_CONPRESION_SIGMA_THR*rms_temp))
  			    {
  			      if(mMapping[mpd_id][adc_ch][1]==0){
  			    	  vRaw_Pedestal_return[mMapping[mpd_id][adc_ch][0]][2][RstripPos]=fadcvalue_temp;
  			      }
  			      if(mMapping[mpd_id][adc_ch][1]==1){
  			    	  vRaw_Pedestal_return[mMapping[mpd_id][adc_ch][0]][12][RstripPos]=fadcvalue_temp;
  			      }
  			      cout<<"orginal strip: "<<stripNb<<" hit in MPD:"<<mpd_id<<" adc:"<<adc_ch<<" strip: "<<RstripNb+1<<"Position: "<<RstripPos<<" adc value:"<<fadcvalue_temp<<" RMS:"<<rms_temp<<endl;
  			      sEventCrossTalk_temp[stripNb]=fadcvalue_temp;
  			      AddressCorrelation[stripNb]=RstripPos;
  			    }
  	      }
  	      map<int,int> sRemoveCrossTalk;
  	      map<int,int> sCrossTalk;
  	      FindCrossTalk(AddressCorrelation,sEventCrossTalk_temp,&sRemoveCrossTalk,&sCrossTalk);
  	      map<int,int>::iterator iter_rmcttemp=sRemoveCrossTalk.begin();
  	      while(iter_rmcttemp!=sRemoveCrossTalk.end()){
  	    	  if(mMapping[mpd_id][adc_ch][1]==0){
  	    		  vRaw_Pedestal_return[mMapping[mpd_id][adc_ch][0]][3][iter_rmcttemp->first]=iter_rmcttemp->second;
  	    	  }
  	    	  if(mMapping[mpd_id][adc_ch][1]==1){
  	    		  vRaw_Pedestal_return[mMapping[mpd_id][adc_ch][0]][13][iter_rmcttemp->first]=iter_rmcttemp->second;
  	    	  }
  	    	  iter_rmcttemp++;
  	      }
  	      map<int,int>::iterator iter_cttemp=sCrossTalk.begin();
  	      while(iter_cttemp!= sCrossTalk.end()) {
  	    	  if(mMapping[mpd_id][adc_ch][1]==0){
  	    		  vRaw_Pedestal_return[mMapping[mpd_id][adc_ch][0]][4][iter_cttemp->first]=iter_cttemp->second;
  	    	  }
  	    	  if(mMapping[mpd_id][adc_ch][1]==1){
  	    		  vRaw_Pedestal_return[mMapping[mpd_id][adc_ch][0]][14][iter_cttemp->first]=iter_cttemp->second;
  	    	  }
  	    	  iter_cttemp++;
  	      }
  	      hMean -> Delete();
  	      hRMS  -> Delete();
  	  }
    }
    return vRaw_Pedestal_return;
}

int InputHandler::HitProcessAllEvents(int entries, string pedestal_file_name, string root_file_name) {

	int entry=0;

	Int_t EvtID,nch,*Vstrip,*VdetID,*VplaneID,*adc0,*adc1,*adc2,*adc3,*adc4,*adc5;
	Vstrip =new Int_t[20000];
	VdetID =new Int_t[20000];
	VplaneID =new Int_t[20000];
	adc0  = new Int_t[20000];
	adc1  = new Int_t[20000];
	adc2  = new Int_t[20000];
	adc3  = new Int_t[20000];
	adc4  = new Int_t[20000];
	adc5  = new Int_t[20000];

	// create the files that used for save the data
	char *HitFilename_temp = new char[100];
	std::strcpy(HitFilename_temp,root_file_name.c_str());
	TFile *Hit_rootfile = new TFile(HitFilename_temp,"RECREATE");
	delete[] HitFilename_temp;

	TTree *Hit = new TTree("GEMHit","Hit list");
	Hit->Branch("evtID",&EvtID,"evtID/I");	     // event ID, start from 1
	Hit->Branch("nch",&nch,"nch/I");                   // how many channels been fired at one time
	Hit->Branch("strip",Vstrip,"strip[nch]/I");	     // number of strips that been fired
	Hit->Branch("detID",VdetID,"detID[nch]/I");        // detector ID ? used for multi-detector case ?  always 0 in this case
	Hit->Branch("planeID",VplaneID,"planeID[nch]/I");  // plan ID

	Hit->Branch("adc0",adc0,"adc0[nch]/I");            // six samples
	Hit->Branch("adc1",adc1,"adc1[nch]/I");
	Hit->Branch("adc2",adc2,"adc2[nch]/I");
	Hit->Branch("adc3",adc3,"adc3[nch]/I");
	Hit->Branch("adc4",adc4,"adc4[nch]/I");
	Hit->Branch("adc5",adc5,"adc5[nch]/I");
	//end of initialize root tree to store hits

	//Loading Mapping
	ifstream filestream (vDefaultMappingPath.c_str(), ifstream::in);
	string line;
	int Mapping_mpdId,Mapping_ADCId,Mapping_I2C,Mapping_GEMId,Mapping_Xis,Mapping_Pos,Mapping_Invert;
	map<int,map<int,vector<int> > > mMapping;
if(filestream.good()){
	while (getline(filestream, line) ) {
	    line.erase( remove_if(line.begin(), line.end(), ::isspace), line.end() );
	    if( line.find("#") == 0 ) continue;
	    char *tokens = strtok( (char *)line.data(), ",");
	    if(tokens !=NULL){
	      cout<<tokens<<" ";Mapping_mpdId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_GEMId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Xis=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_ADCId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_I2C=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Pos=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Invert=atoi(tokens);
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_GEMId);//0
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Xis);//1
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Pos);//2
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Invert);//3
	      //cout<<"test: "<<mMapping[Mapping_mpdId][Mapping_ADCId][2]<<endl;
	    }
	    cout<<endl;
	  }
	 filestream.close();
}else{
	printf("Cannot Loading Mapping\n");
	return -1;
}
	 //end of load Mapping
	 char *PedFilename_temp = new char[100];
	 std::strcpy(PedFilename_temp,pedestal_file_name.c_str());
	 f = new TFile(PedFilename_temp,"READ" );
	 delete[] PedFilename_temp;

	 try {
		 evioFileChannel chan(filename.c_str(),"r");
		 chan.open();
		 while(chan.read()&&entry<entries) {

			 if((entry%200)==0){				// using this function to decrease display refresh interval, increase the process speed
				 Double_t ratio = entry/(Double_t)entries;
				 cout<<setw(8)<<(int)(ratio*100)<<"%\r"<<flush;
			   }

			 // detID   PlaneID  N-strips  SixTimpleSample
			 map<int,map<int,map<int,vector<int> > > >mmHit;
			 vSRSSingleEventData.clear(); // single events data
			 evioDOMTree event(chan);
			 evioDOMNodeListP mpdEventList = event.getNodeList( isLeaf() );
		     evioDOMNodeList::iterator iter;
			 for(iter=mpdEventList->begin();iter!=mpdEventList->end();++iter) {
				 if((*iter)->tag==10) {
					 vector<uint32_t> *vec = (*iter)->getVector<uint32_t>();
					 if(vec!=NULL) {
						 vSRSSingleEventData.reserve(vSRSSingleEventData.size() + vec->size() );
						 vSRSSingleEventData.insert(vSRSSingleEventData.end(), vec->begin(), vec->end() );
					 }
					 else {
						 cout<<"found NULL contents in mpd.."<<endl;
					 }
				 }
			 }

			 if(vSRSSingleEventData.size()!=0) {
				 entry++;
			 }
			 else {
				 continue;
			 }

			 RawDecoder raw_decoder(vSRSSingleEventData);
		     mTsAdc = raw_decoder.GetStripTsAdcMap();//

		     int mpd_id=0;
		     int adc_ch=0;
		     int stripNb=0;

		     map<int,map<int,map<int, vector<int> > > >::iterator it=mTsAdc.begin();
		     for(;it!=mTsAdc.end();++it) {   // loop on all the MPDs
		    	 mpd_id=it->first;
		    	 map<int, map<int, vector<int> > > temp=it->second;
		    	 map<int, map<int, vector<int> > >::iterator itt;

		    	 for(itt=temp.begin();itt!=temp.end();++itt) {      // loop on all the APVs that attached on thi MPD
		    		 adc_ch=itt->first;
		    		 map<int, vector<int> > tempp=itt->second;
		    		 map<int, vector<int> >::iterator ittt;

		    		 //lood the pedestal information fron the root file
		    		 TH1F* hMean = (TH1F*)f->Get(Form("PedestalMean(offset)_mpd_%d_ch_%d",mpd_id, adc_ch));
		    		 TH1F* hRMS  = (TH1F*)f->Get(Form("PedestalRMS_mpd_%d_ch_%d",mpd_id, adc_ch));

		    		 // used for identify the crosstalk issues
		    		 //timeSample  POsition ADC
		    		 map<int, map <int, int> > sAPVCrossTalk_temp;    // raw data,
		    		 map<int, map <int, int> > sAPVAddressCorrelation_temp;	// address correlation
		    		 map<int,int> sAPVCrossTalk_aver_temp;
		    		 map<int,int> sAPVAddressCorrelation_aver_temp;

		    		 for(ittt=tempp.begin(); ittt!=tempp.end(); ++ittt) {
		    			 stripNb=ittt->first;
		    			 vector<int> adc_temp = ittt->second;

		    			 int adcSum_temp=0;
		    			 int TSsize = adc_temp.size();  // sum over the six timesamples
		    			 for(int i =0; i < TSsize; i++) {
		    				 adcSum_temp+=adc_temp[i];
		    			 }
		    			 adcSum_temp=adcSum_temp/TSsize;

		    			 int RstripNb=32*(stripNb%4)+8*(int)(stripNb/4)-31*(int)(stripNb/16);//channel re-matching for apv25 chip

		    			 //stripNb=(8*(int)(stripNb/4)+3-stripNb)*((int)(stripNb/4)%2)+stripNb*(1-((int)(stripNb/4)%2));
		    			 RstripNb=RstripNb+1+RstripNb%4-5*(((int)(RstripNb/4))%2);//channel re-matching for INFN type APV front-end card
		    			 RstripNb=RstripNb+(127-2*RstripNb)*mMapping[mpd_id][adc_ch][3];//re-matching for inverted strips Nb
		    			 int RstripPos;
		    			 RstripPos=RstripNb+128*mMapping[mpd_id][adc_ch][2];// calculate position

		    			 float fadcvalue_temp =adcSum_temp-hMean->GetBinContent(stripNb+1);
		    			 float rms_temp=hRMS->GetBinContent(stripNb+1); // get the sigma for this channels
		    			 if(fadcvalue_temp>ZERO_CONPRESION_SIGMA_THR*rms_temp) {
		    				 int detID=mMapping[mpd_id][adc_ch][0];
		    			     int planeID=mMapping[mpd_id][adc_ch][1];
		    			     for(int i=0; i<TSsize;i++)
		    			      	{
		    			    	 	 sAPVCrossTalk_temp[i].insert(make_pair(RstripPos,adc_temp[i]-hMean->GetBinContent(stripNb+1)));
		    			      	 }
		    			     sAPVCrossTalk_aver_temp[stripNb]=fadcvalue_temp;
		    			     sAPVAddressCorrelation_aver_temp[stripNb]=RstripPos;
		    			 }
		    		 }
		    		 // finish decode one APv
		    		 // save the data and remove the cross talk
		    		 map<int,int> sRemoveCrossTalk;
		    		 map<int,int> sCrossTalk;
		    		 FindCrossTalk(sAPVAddressCorrelation_aver_temp,sAPVCrossTalk_aver_temp,&sRemoveCrossTalk,&sCrossTalk);
		    		 map<int,int>::iterator iter_rmctsave=sRemoveCrossTalk.begin();
		    		 int detID=mMapping[mpd_id][adc_ch][0];
		    		 int planeID=mMapping[mpd_id][adc_ch][1];
		    		 while(iter_rmctsave!=sRemoveCrossTalk.end()) {

		    			 int NtimeSample_temp=sAPVCrossTalk_temp.size();
		    			 for(int i =0; i < NtimeSample_temp ; i++) {
		    				 mmHit[detID][planeID][iter_rmctsave->first].push_back(sAPVCrossTalk_temp[i][iter_rmctsave->first]);
		    			 }
		    			 iter_rmctsave++;
		    		 }
		    		 sAPVCrossTalk_temp.clear();
		    		 sAPVAddressCorrelation_temp.clear();
		    		 sAPVCrossTalk_aver_temp.clear();
		    		 sAPVAddressCorrelation_aver_temp.clear();
		    		 //finish save the data
		    		 hMean->Delete();
		    		 hRMS->Delete();
		    	 }  // end of loop on APVs
		     }      // end of loop on MPDs -finish decode one event

		     int detid,planeid;
		     Int_t nstrip=0;
		     for(map<int, map<int, map<int, vector<int> > > >::iterator it = mmHit.begin(); it!=mmHit.end(); ++it) {
		    	 detid = it->first;
		    	 map<int, map<int, vector<int> > >temp = it->second;
		    	 map<int, map<int, vector<int> > >::iterator itt;
		    	 for(itt=temp.begin(); itt!=temp.end(); ++itt) {
		    		 planeid = itt->first;
		    		 vector<int> stripVector;//cout<<"planeid: "<<planeid<<"  "<<stripVector.size()<<"DDDDD"<<endl;
		    		 map<int, vector<int> > tempp = itt->second;
		    		 map<int, vector<int> > ::iterator ittt;
		    		 for(ittt=tempp.begin(); ittt!=tempp.end(); ++ittt) { //cout<<"HHH"<<ittt->first<<endl;
		    			 stripVector.push_back(ittt->first);
		    		 }
		    		 //cout<<"planeid: "<<planeid<<"  "<<stripVector.size()<<"asdasd"<<endl;
		    		 sort(stripVector.begin(),stripVector.end());
		    		 for(unsigned int i=0;i<stripVector.size();i++) {
		    			 Vstrip[nstrip]=stripVector[i];
		    			 adc0[nstrip]=mmHit[detid][planeid][stripVector[i]][0];
		    			 adc1[nstrip]=mmHit[detid][planeid][stripVector[i]][1];
		    			 adc2[nstrip]=mmHit[detid][planeid][stripVector[i]][2];
		    			 adc3[nstrip]=mmHit[detid][planeid][stripVector[i]][3];
		    			 adc4[nstrip]=mmHit[detid][planeid][stripVector[i]][4];
		    			 adc5[nstrip]=mmHit[detid][planeid][stripVector[i]][5];
		    			 //cout<<"ADC:"<<stripVector[i]<<" "<<adc0[nstrip]<<" "<<adc1[nstrip]<<" "<<adc2[nstrip]<<" "<<adc3[nstrip]<<" "<<adc4[nstrip]<<" "<<adc5[nstrip]<<endl;
		    			 VdetID[nstrip]=detid;
		    			 VplaneID[nstrip]=planeid;
		    			 nstrip++;
		    		 }
		    	 }
		     }
		     nch=nstrip;
		     EvtID=entry;
		     Hit->Fill();
		 }
		 chan.close();
	} catch (evioException e) {
		cerr<<endl<<e.toString()<<endl<<endl;
		exit(EXIT_FAILURE);
	}
	f->Close();

	int mpd_id=0;
	int adc_ch=0;
	int stripNb=0;

	Hit->Write();
	Hit_rootfile->Write();
	Hit_rootfile->Close();
	delete[] Vstrip;
	delete[] VdetID;
	delete[] VplaneID;
	delete[] adc0 ;
	delete[] adc1 ;
	delete[] adc2 ;
	delete[] adc3 ;
	delete[] adc4 ;
	delete[] adc5 ;
	//exit(EXIT_SUCCESS);
	return entry;
}

int InputHandler::HitProcessAllEvents(string pedestal_file_name, string root_file_name) {

	int entry=0;
	Int_t EvtID,nch,*Vstrip,*VdetID,*VplaneID,*adc0,*adc1,*adc2,*adc3,*adc4,*adc5;
	Vstrip =new Int_t[5000];
	VdetID =new Int_t[5000];
	VplaneID =new Int_t[5000];
	adc0  = new Int_t[5000];
	adc1  = new Int_t[5000];
	adc2  = new Int_t[5000];
	adc3  = new Int_t[5000];
	adc4  = new Int_t[5000];
	adc5  = new Int_t[5000];
	// create the files that used for save the data
	char *HitFilename_temp = new char[100];
	std::strcpy(HitFilename_temp,root_file_name.c_str());

	TThread::Lock();
	TFile *Hit_rootfile = new TFile(HitFilename_temp,"RECREATE");
	TThread::UnLock();

	delete[] HitFilename_temp;

	TThread::Lock();
	TTree *Hit = new TTree("GEMHit","Hit list");
	Hit->Branch("evtID",&EvtID,"evtID/I");	     // event ID, start from 1
	Hit->Branch("nch",&nch,"nch/I");                   // how many channels been fired at one time
	Hit->Branch("strip",Vstrip,"strip[nch]/I");	     // number of strips that been fired
	Hit->Branch("detID",VdetID,"detID[nch]/I");        // detector ID ? used for multi-detector case ?  always 0 in this case
	Hit->Branch("planeID",VplaneID,"planeID[nch]/I");  // plan ID

	Hit->Branch("adc0",adc0,"adc0[nch]/I");            // six samples
	Hit->Branch("adc1",adc1,"adc1[nch]/I");
	Hit->Branch("adc2",adc2,"adc2[nch]/I");
	Hit->Branch("adc3",adc3,"adc3[nch]/I");
	Hit->Branch("adc4",adc4,"adc4[nch]/I");
	Hit->Branch("adc5",adc5,"adc5[nch]/I");
	TThread::UnLock();
	//end of initialize root tree to store hits
	//Loading Mapping
	ifstream filestream (vDefaultMappingPath.c_str(), ifstream::in);
	string line;
	int Mapping_mpdId,Mapping_ADCId,Mapping_I2C,Mapping_GEMId,Mapping_Xis,Mapping_Pos,Mapping_Invert;
	map<int,map<int,vector<int> > > mMapping;
if(filestream.good()){
	while (getline(filestream, line) ) {
	    line.erase( remove_if(line.begin(), line.end(), ::isspace), line.end() );
	    if( line.find("#") == 0 ) continue;
	    char *tokens = strtok( (char *)line.data(), ",");
	    if(tokens !=NULL){
	      cout<<tokens<<" ";Mapping_mpdId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_GEMId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Xis=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_ADCId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_I2C=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Pos=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Invert=atoi(tokens);
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_GEMId);//0
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Xis);//1
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Pos);//2
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Invert);//3
	      //cout<<"test: "<<mMapping[Mapping_mpdId][Mapping_ADCId][2]<<endl;
	    }
	    cout<<endl;
	  }
	 filestream.close();
}else{
	printf("Cannot loading Mapping\n");
	return -1;
}
	 //end of load Mapping
	 char *PedFilename_temp = new char[100];
	 std::strcpy(PedFilename_temp,pedestal_file_name.c_str());
	 f = new TFile(PedFilename_temp,"READ" );
	 delete[] PedFilename_temp;
	 try {
		 evioFileChannel chan(filename.c_str(),"r");
		 chan.open();
		 while(chan.read()) {
			 // detID   PlaneID  N-strips  SixTimpleSample
			 map<int,map<int,map<int,vector<int> > > >mmHit;
			 vSRSSingleEventData.clear(); // single events data
			 evioDOMTree event(chan);
			 evioDOMNodeListP mpdEventList = event.getNodeList( isLeaf() );
		     evioDOMNodeList::iterator iter;
			 for(iter=mpdEventList->begin();iter!=mpdEventList->end();++iter) {
				 if((*iter)->tag==10) {
					 vector<uint32_t> *vec = (*iter)->getVector<uint32_t>();
					 if(vec!=NULL) {
						 vSRSSingleEventData.reserve(vSRSSingleEventData.size() + vec->size() );
						 vSRSSingleEventData.insert(vSRSSingleEventData.end(), vec->begin(), vec->end() );
					 }
					 else {
						 cout<<"found NULL contents in mpd.."<<endl;
					 }
				 }
			 }

			 if(vSRSSingleEventData.size()!=0) {
				 entry++;
			 }
			 else {
				 continue;
			 }
			 RawDecoder raw_decoder(vSRSSingleEventData);
		     mTsAdc = raw_decoder.GetStripTsAdcMap();//

		     int mpd_id=0;
		     int adc_ch=0;
		     int stripNb=0;

		     map<int,map<int,map<int, vector<int> > > >::iterator it=mTsAdc.begin();
		     for(;it!=mTsAdc.end();++it) {   // loop on all the MPDs
		    	 mpd_id=it->first;
		    	 map<int, map<int, vector<int> > > temp=it->second;
		    	 map<int, map<int, vector<int> > >::iterator itt;

		    	 for(itt=temp.begin();itt!=temp.end();++itt) {      // loop on all the APVs that attached on thi MPD
		    		 adc_ch=itt->first;

		    		 // in this if condition make sure current cards is in the mapping files, if it is not in the mapping exclude it
		    		 if((mMapping.find(mpd_id)!=mMapping.end())&&(mMapping[mpd_id].find(adc_ch)!=mMapping[mpd_id].end()))
		    		 {
		    		 map<int, vector<int> > tempp=itt->second;
		    		 map<int, vector<int> >::iterator ittt;

		    		 //lood the pedestal information fron the root file
		    		 TThread::Lock();
		    		 TH1F* hMean = (TH1F*)f->Get(Form("PedestalMean(offset)_mpd_%d_ch_%d",mpd_id, adc_ch));
		    		 TH1F* hRMS  = (TH1F*)f->Get(Form("PedestalRMS_mpd_%d_ch_%d",mpd_id, adc_ch));
		    		 TThread::UnLock();

		    		 // used for identify the crosstalk issues
		    		 //timeSample  POsition ADC
		    		 map<int, map <int, int> > sAPVCrossTalk_temp;    // raw data,
		    		 map<int, map <int, int> > sAPVAddressCorrelation_temp;	// address correlation
		    		 map<int,int> sAPVCrossTalk_aver_temp;
		    		 map<int,int> sAPVAddressCorrelation_aver_temp;

		    		 for(ittt=tempp.begin(); ittt!=tempp.end(); ++ittt) {
		    			 stripNb=ittt->first;
		    			 vector<int> adc_temp = ittt->second;

		    			 int adcSum_temp=0;
		    			 int TSsize = adc_temp.size();  // sum over the six timesamples
		    			 for(int i =0; i < TSsize; i++) {
		    				 adcSum_temp+=adc_temp[i];
		    			 }
		    			 adcSum_temp=adcSum_temp/TSsize;

		    			 int RstripNb=32*(stripNb%4)+8*(int)(stripNb/4)-31*(int)(stripNb/16);//channel re-matching for apv25 chip

		    			 //stripNb=(8*(int)(stripNb/4)+3-stripNb)*((int)(stripNb/4)%2)+stripNb*(1-((int)(stripNb/4)%2));
		    			 RstripNb=RstripNb+1+RstripNb%4-5*(((int)(RstripNb/4))%2);//channel re-matching for INFN type APV front-end card
		    			 RstripNb=RstripNb+(127-2*RstripNb)*mMapping[mpd_id][adc_ch][3];//re-matching for inverted strips Nb
		    			 int RstripPos;
		    			 RstripPos=RstripNb+128*mMapping[mpd_id][adc_ch][2];// calculate position

		    			 float fadcvalue_temp =adcSum_temp-hMean->GetBinContent(stripNb+1);
		    			 float rms_temp=hRMS->GetBinContent(stripNb+1); // get the sigma for this channels

		    			 if(fadcvalue_temp>ZERO_CONPRESION_SIGMA_THR*rms_temp) {

		    				 int detID=mMapping[mpd_id][adc_ch][0];
		    			     int planeID=mMapping[mpd_id][adc_ch][1];
		    			     for(int i=0; i<TSsize;i++)
		    			      	{
		    			    	 	 sAPVCrossTalk_temp[i].insert(make_pair(RstripPos,adc_temp[i]-hMean->GetBinContent(stripNb+1)));
		    			      	 }
		    			     sAPVCrossTalk_aver_temp[stripNb]=fadcvalue_temp;
		    			     sAPVAddressCorrelation_aver_temp[stripNb]=RstripPos;

		    			 }
		    		 }
		    		 // finish decode one APv
		    		 // save the data and remove the cross talk
		    		 map<int,int> sRemoveCrossTalk;
		    		 map<int,int> sCrossTalk;
		    		 FindCrossTalk(sAPVAddressCorrelation_aver_temp,sAPVCrossTalk_aver_temp,&sRemoveCrossTalk,&sCrossTalk);
		    		 map<int,int>::iterator iter_rmctsave=sRemoveCrossTalk.begin();
		    		 int detID=mMapping[mpd_id][adc_ch][0];
		    		 int planeID=mMapping[mpd_id][adc_ch][1];
		    		 while(iter_rmctsave!=sRemoveCrossTalk.end()) {

		    			 int NtimeSample_temp=sAPVCrossTalk_temp.size();
		    			 for(int i =0; i < NtimeSample_temp ; i++) {
		    				 mmHit[detID][planeID][iter_rmctsave->first].push_back(sAPVCrossTalk_temp[i][iter_rmctsave->first]);
		    			 }
		    			 iter_rmctsave++;
		    		 }
		    		 sAPVCrossTalk_temp.clear();
		    		 sAPVAddressCorrelation_temp.clear();
		    		 sAPVCrossTalk_aver_temp.clear();
		    		 sAPVAddressCorrelation_aver_temp.clear();
		    		 //finish save the data
		    		 hMean->Delete();
		    		 hRMS->Delete();
		    	 }  // end of loop on APVs

		     }   // except the cards that not in the mapping

		     }      // end of loop on MPDs -finish decode one event
		     int detid,planeid;
		     Int_t nstrip=0;
		     for(map<int, map<int, map<int, vector<int> > > >::iterator it = mmHit.begin(); it!=mmHit.end(); ++it) {
		    	 detid = it->first;
		    	 map<int, map<int, vector<int> > >temp = it->second;
		    	 map<int, map<int, vector<int> > >::iterator itt;
		    	 for(itt=temp.begin(); itt!=temp.end(); ++itt) {
		    		 planeid = itt->first;
		    		 vector<int> stripVector;//cout<<"planeid: "<<planeid<<"  "<<stripVector.size()<<"DDDDD"<<endl;
		    		 map<int, vector<int> > tempp = itt->second;
		    		 map<int, vector<int> > ::iterator ittt;
		    		 for(ittt=tempp.begin(); ittt!=tempp.end(); ++ittt) { //cout<<"HHH"<<ittt->first<<endl;
		    			 stripVector.push_back(ittt->first);
		    		 }
		    		 //cout<<"planeid: "<<planeid<<"  "<<stripVector.size()<<"asdasd"<<endl;
		    		 sort(stripVector.begin(),stripVector.end());
		    		 for(unsigned int i=0;i<stripVector.size();i++) {
		    			 Vstrip[nstrip]=stripVector[i];
		    			 adc0[nstrip]=mmHit[detid][planeid][stripVector[i]][0];
		    			 adc1[nstrip]=mmHit[detid][planeid][stripVector[i]][1];
		    			 adc2[nstrip]=mmHit[detid][planeid][stripVector[i]][2];
		    			 adc3[nstrip]=mmHit[detid][planeid][stripVector[i]][3];
		    			 adc4[nstrip]=mmHit[detid][planeid][stripVector[i]][4];
		    			 adc5[nstrip]=mmHit[detid][planeid][stripVector[i]][5];
		    			 //cout<<"ADC:"<<stripVector[i]<<" "<<adc0[nstrip]<<" "<<adc1[nstrip]<<" "<<adc2[nstrip]<<" "<<adc3[nstrip]<<" "<<adc4[nstrip]<<" "<<adc5[nstrip]<<endl;
		    			 VdetID[nstrip]=detid;
		    			 VplaneID[nstrip]=planeid;
		    			 nstrip++;
		    		 }
		    	 }
		     }
		     nch=nstrip;
		     EvtID=entry;
		     Hit->Fill();
		 }
		 chan.close();
	} catch (evioException e) {
		cerr<<endl<<e.toString()<<endl<<endl;
		exit(EXIT_FAILURE);
	}
	f->Close();

	int mpd_id=0;
	int adc_ch=0;
	int stripNb=0;

	TThread::Lock();
	Hit->Write();
	Hit_rootfile->Write();
	Hit_rootfile->Close();
	TThread::UnLock();

	delete[] Vstrip;
	delete[] VdetID;
	delete[] VplaneID;
	delete[] adc0 ;
	delete[] adc1 ;
	delete[] adc2 ;
	delete[] adc3 ;
	delete[] adc4 ;
	delete[] adc5 ;
	//exit(EXIT_SUCCESS);
	return entry;
}


int InputHandler::ZeroSProcessAllEvents(int entries, string pedestal_file_name) {
	cout<<pedestal_file_name.data()<<endl;
	int entry=0;

	// visualize
	TCanvas *cHit = new TCanvas("cHit", "ZeroSupression", 10, 10, 1200, 1000);
	cHit->Divide(2,3);

	//Initialize root tree to store hits
	//TFile *Hit_rootfile = new TFile("13apv_prod12Hit.root","RECREATE");
	Int_t EvtID,nch,*Vstrip,*VdetID,*VplaneID,*adc0,*adc1,*adc2,*adc3,*adc4,*adc5;
	//end of initialize root tree to store hits
	//Loading Mapping
	ifstream filestream (vDefaultMappingPath.c_str(), ifstream::in);
	string line;
	int Mapping_mpdId,Mapping_ADCId,Mapping_I2C,Mapping_GEMId,Mapping_Xis,Mapping_Pos,Mapping_Invert;
	map<int,map<int,vector<int> > > mMapping;
	if(filestream.good()){
	while (getline(filestream, line) ) {
	    line.erase( remove_if(line.begin(), line.end(), ::isspace), line.end() );
	    if( line.find("#") == 0 ) continue;
	    char *tokens = strtok( (char *)line.data(), ",");
	    if(tokens !=NULL){
	      cout<<tokens<<" ";Mapping_mpdId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_GEMId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Xis=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_ADCId=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_I2C=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Pos=atoi(tokens);
	      tokens = strtok(NULL, " ,");cout<<tokens<<" ";Mapping_Invert=atoi(tokens);
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_GEMId);  //0
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Xis);    //1
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Pos);    //2
	      mMapping[Mapping_mpdId][Mapping_ADCId].push_back(Mapping_Invert); //3
	      //cout<<"test: "<<mMapping[Mapping_mpdId][Mapping_ADCId][2]<<endl;
	    }
	    cout<<endl;
	  }
	  filestream.close();
	}else{
		printf("Cannot Loading Mapping\n");
		return -1;
	}

	  //end of load Mapping

	  char *PedFilename_temp = new char[100];
	  std::strcpy(PedFilename_temp,pedestal_file_name.c_str());    // load the pedestal file
	  f = new TFile(PedFilename_temp,"READ" );
	  delete[] PedFilename_temp;

	  try {

		  evioFileChannel chan(filename.c_str(),"r"); // read the .dat file
		  chan.open();
		  while(chan.read() && entry<entries) {
			  map<int, map<int, map<int, vector<int> > > > mmHit;
			  //vector<uint32_t> srsSingleEvent;
			  vSRSSingleEventData.clear();

			  evioDOMTree event(chan);
			  evioDOMNodeListP mpdEventList = event.getNodeList( isLeaf() );
			  cout<<"number of banks: "<<mpdEventList->size()<<endl;

			  evioDOMNodeList::iterator iter;
			  for (iter = mpdEventList->begin(); iter != mpdEventList->end(); ++iter) {
				  if(((*iter)->tag)==10) {
					  vector<uint32_t> *vec = (*iter)->getVector<uint32_t>();
					  if(vec!=NULL)
					  	{
					  	    vSRSSingleEventData.reserve(vSRSSingleEventData.size() + vec->size() );
					  	    vSRSSingleEventData.insert(vSRSSingleEventData.end(), vec->begin(), vec->end() );
					  	 }
					  	else
					  	 {
					  	    cout<<"found NULL contents in mpd.."<<endl;
					  	  }
				  }
			   }

			  cout<<"Event ID: "<<entry<<endl;
			  if(vSRSSingleEventData.size()!=0){
				  entry++;
			  }
			  else{
				  continue;
			  }
			  cout<<"Event Size [uint_32 in number of words]:"<<vSRSSingleEventData.size()<<endl;

			  RawDecoder raw_decoder(vSRSSingleEventData);
			  //map<mpd_id, map<ch_id, vector<int> > >, vector: adc values (all time samples)
			  mTsAdc = raw_decoder.GetStripTsAdcMap();//

			  int mpd_id=0;
			  int adc_ch=0;
			  int stripNb=0;

			  // zero supression

		      TH1F* hX = new TH1F("hitX","hitX",1537,0,1536);
		      TH1F* hhX = new TH1F("beforeZeroSupressionX","beforeZeroSupressionX",1537,0,1536);
		      TH1F* hY = new TH1F("hitY","hitY",1501,0,1500);//500,1500);
		      TH1F* hhY = new TH1F("beforeZeroSupressionY","beforeZeroSupressionY",1501,0,1500);//500,1500);

		      // to be improve
		      TH1F *hX_rmCt=new TH1F("hitX_rmct","hitX_rmct",1537,0,1536);
		      TH1F *hY_rmCt=new TH1F("hitY_rmct","hitY_rmct",1501,0,1500);

		      TH1F *hX_Ct=new TH1F("hitX_ct","hitX_ct",1537,0,1536);
		      hX_Ct->SetLineColor(2);
		      TH1F *hY_Ct=new TH1F("hitY_ct","hitY_ct",1501,0,1500);
		      hY_Ct->SetLineColor(2);

			  //MPD_ID,    ADC_channels stripsNB   six timesample
		      for(map<int, map<int, map<int,vector<int> > > >::iterator it = mTsAdc.begin(); it!=mTsAdc.end(); ++it) {
		    	  mpd_id=it->first;
		    	  map<int, map<int, vector<int> > > temp=it->second;

		    	  map<int, map<int,vector<int> > >::iterator itt;
		    	  for(itt=temp.begin(); itt!=temp.end(); ++itt) {
		    	      adc_ch = itt->first;
		    	      map<int,vector<int> > tempp = itt->second;
		    	      map<int,vector<int> >::iterator ittt;

		    	      //loading pedestal information from root files
		    	      TH1F* hMean = (TH1F*)f->Get(Form("PedestalMean(offset)_mpd_%d_ch_%d",mpd_id, adc_ch));  // read the pedestal
		    	      TH1F* hRMS  = (TH1F*)f->Get(Form("PedestalRMS_mpd_%d_ch_%d",mpd_id, adc_ch));			  // read the pedestal
		    	      // loop an all the Strips
		    	      // added by Siyu, To be improve
		    	      map<int,int> sEventCrossTalk_temp;
		    	      map<int,int> AddressCorrelation;

		    	      for(ittt=tempp.begin();ittt!=tempp.end();++ittt) {
		    			  stripNb = ittt->first;

		    			  vector<int> adc_temp = ittt->second;   // buffer the six(or other) Timesample ADC value
		    			  int adcSum_temp=0;
		    			  int TSsize = adc_temp.size();
		    			  for(int i=0; i<TSsize;i++)
		    			    { adcSum_temp+=adc_temp[i];
		    			    }
		    			  adcSum_temp=adcSum_temp/TSsize;  // get the mean of the six time sample

		    			  // convert the address to the real strips address
		    			  int RstripNb=32*(stripNb%4)+8*(int)(stripNb/4)-31*(int)(stripNb/16);//channel re-matching for apv25 chip
		    			  int RstripPos;

		    			  //stripNb=(8*(int)(stripNb/4)+3-stripNb)*((int)(stripNb/4)%2)+stripNb*(1-((int)(stripNb/4)%2));
		    			  RstripNb=RstripNb+1+RstripNb%4-5*(((int)(RstripNb/4))%2);//channel re-matching for INFN type APV front-end card, ???????
		    			  //invert the address if needed
		    			  RstripNb=RstripNb+(127-2*RstripNb)*mMapping[mpd_id][adc_ch][3];//re-matching for inverted strips Nb
		    			  RstripPos=RstripNb+128*mMapping[mpd_id][adc_ch][2];            // calculate position

		    			  float fadcvalue_temp = adcSum_temp-hMean->GetBinContent(stripNb+1);   // zero subtraction
		    			  float rms_temp=hRMS->GetBinContent(stripNb+1);
		    			  if(mMapping[mpd_id][adc_ch][1]==0){hhX->Fill(RstripPos,fadcvalue_temp);}
		    			  if(mMapping[mpd_id][adc_ch][1]==1){hhY->Fill(RstripPos,fadcvalue_temp);}

		    			  // apply 5 sigma select the effective hit
		    			  if(( fadcvalue_temp>ZERO_CONPRESION_SIGMA_THR*rms_temp))
		    			    {
		    			      int detID=mMapping[mpd_id][adc_ch][0];
		    			      int planeID=mMapping[mpd_id][adc_ch][1];
//		    			      for(int i=0; i<TSsize;i++)   // save the six time sample seperately
//		    				   {
//		    			    	 //apply zero suptraction to all the six time samples
//		    				     mmHit[detID][planeID][RstripPos].push_back(adc_temp[i]-hMean->GetBinContent(stripNb+1));
//		    				    }
		    			      if(mMapping[mpd_id][adc_ch][1]==0){hX->Fill(RstripPos,fadcvalue_temp);}
		    			      if(mMapping[mpd_id][adc_ch][1]==1){hY->Fill(RstripPos,fadcvalue_temp);}
		    			      cout<<"orginal strip: "<<stripNb<<" hit in MPD:"<<mpd_id<<" adc:"<<adc_ch<<" strip: "<<RstripNb+1<<"Position: "<<RstripPos<<" adc value:"<<fadcvalue_temp<<" RMS:"<<rms_temp<<endl;
		    			      sEventCrossTalk_temp[stripNb]=fadcvalue_temp;
		    			      AddressCorrelation[stripNb]=RstripPos;
		    			    }
		    			  //hhh->Delete();
		    	      }
		    	      map<int,int> sRemoveCrossTalk;
		    	      map<int,int> sCrossTalk;
		    	      FindCrossTalk(AddressCorrelation,sEventCrossTalk_temp,&sRemoveCrossTalk,&sCrossTalk);
		    	      //cout<<" crosstalk size="<<sRemoveCrossTalk.size()<<" crosstalk size="<<sCrossTalk.size()<<endl;
		    	      map<int,int>::iterator iter_rmcttemp=sRemoveCrossTalk.begin();
		    	      while(iter_rmcttemp!=sRemoveCrossTalk.end()){
		    	    	  if(mMapping[mpd_id][adc_ch][1]==0){hX_rmCt->Fill(iter_rmcttemp->first,iter_rmcttemp->second);}
		    	    	  if(mMapping[mpd_id][adc_ch][1]==1){hY_rmCt->Fill(iter_rmcttemp->first,iter_rmcttemp->second);}
		    	    	  iter_rmcttemp++;
		    	      }
		    	      map<int,int>::iterator iter_cttemp=sCrossTalk.begin();
		    	      while(iter_cttemp!= sCrossTalk.end()) {
		    	    	  if(mMapping[mpd_id][adc_ch][1]==0){hX_Ct->Fill(iter_cttemp->first,iter_cttemp->second);}
		    	    	  if(mMapping[mpd_id][adc_ch][1]==1){hY_Ct->Fill(iter_cttemp->first,iter_cttemp->second);}
		    	    	  iter_cttemp++;
		    	      }
		    	      hMean -> Delete();
		    	      hRMS  -> Delete();
		    	  }

		      }
		      cHit->cd(1);
		      hX_rmCt->Draw();
		      hX_Ct->Draw("same");
		      cHit->cd(3);
		      hX->Draw();
		      cHit->cd(5);
		      hhX->Draw();

		      cHit->cd(2);
		      hY_rmCt->Draw();
		      hY_Ct->Draw("same");
		      cHit->cd(4);
		      hY->Draw();
		      cHit->cd(6);
		      hhY->Draw();
		      cHit->Update();
		      getchar();
		      // right place for the display
		      hX->Delete();
		      hhX->Delete();
		      hY->Delete();
		      hhY->Delete();
		      hX_Ct->Delete();
		      hX_rmCt->Delete();
		      hY_Ct->Delete();
		      hY_rmCt->Delete();

		  }
		  chan.close();
	    }
	    catch (evioException e) {
		   cerr<< endl << e.toString() << endl <<endl;
		   exit(EXIT_FAILURE);
	    }

	f->Close();
	//delete cHit;
	return 0;
};

// seperat the real signal and the cross talk signal
//input
// AddressCorrelation:  stripNb, RstripPos  (stripID(original), transfered address(all MPD))
// sEvent_input:        sigal Events sigal APV singal Time sample <stripNb, ADC>  after zero supression
// return value
// sRemoveCrossTalk;    real hit after remove the cross talk
// sCrossTalk: 			cross talk events
int InputHandler::FindCrossTalk(map<int,int> Address_Correlation,map<int, int > sEvent_input,map<int, int > *sRemoveCrossTalk,map<int, int> *sCrossTalk){
	vector<map<int,int> > AllEvent_buffer; //   find all the condidate crosstalk events, from the raw address, corss talk should near each other
	vector<map<int,int> > sRemoveCrossTalk_buffer;
//	vector<map<int,int> > sRemoveCrossTalk_level1;
	vector<map<int,int> > CrossTalk_condidate_buffer;

	//++++++ seperate the fake hit, if the number of fired strips more than one, then this should be a corsstalk candidate
	if(sEvent_input.size()!=0) {

		// searching for the connectd histograms
		map<int,int>::iterator iter_raw=sEvent_input.begin();
		map<int,int> Connected_addr_temp;
		int Pre_addr_buffer=-1;
		int test= 0;
		while(iter_raw!=sEvent_input.end()) {

			if(Pre_addr_buffer==(iter_raw->first -1)){
				Connected_addr_temp.insert(make_pair(iter_raw->first,iter_raw->second));
				Pre_addr_buffer=iter_raw->first;
			}
			else {
				if(Connected_addr_temp.size()!=0)
				 {
					AllEvent_buffer.push_back(Connected_addr_temp);
					//Connected_addr_temp.clear();
				 }
				Connected_addr_temp.clear();
				Connected_addr_temp.insert(make_pair(iter_raw->first,iter_raw->second));
				Pre_addr_buffer=iter_raw->first;
			}
			iter_raw++;
			if(iter_raw==sEvent_input.end())AllEvent_buffer.push_back(Connected_addr_temp);
		}
	}
	else return -1;
	//++++++++++++
	//+++++ select the crosstalk and the effective events+++++
	// write the cross talk and non-talk in the buffer
	vector<map<int,int> >::iterator iter_allevents=AllEvent_buffer.begin();
	for(;iter_allevents!=AllEvent_buffer.end();iter_allevents++) {

		if((*iter_allevents).size()==1) {	// if no raw strips is connected, it cannot be a cross talk
			sRemoveCrossTalk_buffer.push_back(*iter_allevents);
		}
		else {

			// find the fake maximum for each fake cluster
			map<int,int>::iterator itter_cluster=(*iter_allevents).begin();
			int MaxADC_address_temp=-1;
			int MaxADC_temp=-1;
			while(itter_cluster!=(*iter_allevents).end()) {
				if(itter_cluster->second > MaxADC_temp) {
					MaxADC_temp=itter_cluster->second;
					MaxADC_address_temp=itter_cluster->first;
				}
				itter_cluster++;
			}

			// the maximum of the fake cluster would be the reall signal
			map<int,int> Cluster_rmct;
			Cluster_rmct.insert(make_pair(MaxADC_address_temp,MaxADC_temp));
			sRemoveCrossTalk_buffer.push_back(Cluster_rmct);

			// the strips near the maximum would be the crosstalk condidate
			// write the others in the crosstalk candidate buffer
			map<int,int> CrossCandidate_maptemp=(*iter_allevents);
			CrossCandidate_maptemp.erase(MaxADC_address_temp);
			CrossTalk_condidate_buffer.push_back(CrossCandidate_maptemp);
		}
	}
	//+++++++++++++
	/*// test functions
	vector<map<int,int > >::iterator iter_temp=AllEvent_buffer.begin();
	for(;iter_temp!=AllEvent_buffer.end();iter_temp++) {
		map<int,int>::iterator itter_temp=(*iter_temp).begin();
		while(itter_temp!=(*iter_temp).end()) {
			cout<<"  **address="<<itter_temp->first<<" Pos="<<Address_Correlation[itter_temp->first]<<"  ADC="<<itter_temp->second<<endl;
			itter_temp++;
		}
		cout<<endl;
	}
	vector<map<int,int> >::iterator iter_test=sRemoveCrossTalk_buffer.begin();
	for(;iter_test!=sRemoveCrossTalk_buffer.end();iter_test++) {
		map<int,int>::iterator iter_maptest=(*iter_test).begin();
		while (iter_maptest!=(*iter_test).end()) {
			cout<<"   *=> reall add="<< iter_maptest->first<< " pos="<<Address_Correlation[iter_maptest->first]<<"  adc="<<iter_maptest->second<<endl;
			iter_maptest++;
		}
	}
	cout<<" *crosstalk signal"<<endl;
	iter_test=CrossTalk_condidate_buffer.begin();
	for(;iter_test!=CrossTalk_condidate_buffer.end();iter_test++){
		map<int,int>::iterator iter_maptest=(*iter_test).begin();
		while(iter_maptest!=(*iter_test).end()) {
			cout<<"   *=> Ctalk add="<< iter_maptest->first<< " pos="<<Address_Correlation[iter_maptest->first]<<"  adc="<<iter_maptest->second<<endl;
			iter_maptest++;
		}
	}*/
	//end of test function
	//++++++++++++++
	// save the generated function into the return buffers
	vector<map<int,int> >::iterator iter_vec_save=sRemoveCrossTalk_buffer.begin();
	for(;iter_vec_save !=sRemoveCrossTalk_buffer.end(); iter_vec_save++) {
		map<int,int>::iterator itter_map_save=(*iter_vec_save).begin();
		while(itter_map_save!=(*iter_vec_save).end()){
			(*sRemoveCrossTalk).insert(make_pair(Address_Correlation[itter_map_save->first],itter_map_save->second));
			itter_map_save++;
		}
	}
	iter_vec_save=CrossTalk_condidate_buffer.begin();
	for(;iter_vec_save!=CrossTalk_condidate_buffer.end();iter_vec_save++) {
		map<int,int>::iterator itter_map_save=(*iter_vec_save).begin();
		while(itter_map_save!=(*iter_vec_save).end()) {
			(*sCrossTalk).insert(make_pair(Address_Correlation[itter_map_save->first],itter_map_save->second));
			itter_map_save++;
		}
	}
	//++++++++++++++
	return 1;
};

InputHandler::~InputHandler() {
	// TODO Auto-generated destructor stub
	int mpd_id,adc_ch,stripNb;
	map<int, map<int, map<int,vector<int> > > >::iterator it;
	for(it = mTsAdc.begin(); it!=mTsAdc.end(); ++it)
	  {
	    mpd_id = it->first;
	    map<int, map<int,vector<int> > > temp = it->second;

	    map<int, map<int,vector<int> > >::iterator itt;
	    for(itt=temp.begin(); itt!=temp.end(); ++itt)
		 {
		   adc_ch = itt->first;
		   map<int,vector<int> > tempp = itt->second;
		   map<int,vector<int> >::iterator ittt;

		   for(ittt=tempp.begin(); ittt!=tempp.end(); ++ittt) {
		      stripNb = ittt->first;
		    }
		  }
	    }
	  mAPVRawHistos.clear();
	  mAPVRawTSs.clear();
}



// test functions used for test usage, add by siyu
int InputHandler::TestFunction() {
	cout<<"asa"<<endl;
	//HitProcessAllEvents(100000,"/home/newdriver/Research/SBS/SBS_GEM_labtest/Decoder_Result/SBS35/Pedestal/SBS35_Pedestal_1110.root","test.root");
	//ZeroSProcessAllEvents(10000,"/home/newdriver/Research/SBS/SBS_GEM_labtest/Decoder_Result/SBS35/Pedestal/SBS35_Pedestal_1110.root");
};
