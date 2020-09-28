/*
  程序运行后，能录制音乐，之后能播放音乐
  最多录制两首
*/
#include <SDHCI.h>
#include <Audio.h>

//#define RecordLoopNum 200
#define REC_ROOT_PATH "RECORD"

enum
{
  PlayReady = 0,
  Playing,
  RecordReady,//start
  Recording,//持续录音 ==》 recordStream
  None,//空闲状态
  Remove,
  ChoseRemove,
  ChosePlay
};

SDClass theSD;
AudioClass *theAudio;


int DB_value=-300;

int fileCount = 0;
File currentFile;
File currentDir;


void setup()
{
  Serial.begin(115200);
  Serial.println("Serial SetUp!");
  initAudio();
  init_SD();
}

void init_SD(){
    currentDir = theSD.open(REC_ROOT_PATH);
    printf("currentDir=%s\n",currentDir.name());
    if(!currentDir){
        printf("Err:dir not exits\n");  
    }else{
        printf("dir open ok\n");  
    }
    countFile();
}

void countFile(){
    File tmpFile;
    while((tmpFile=currentDir.openNextFile())!=NULL){
        fileCount++;
        printf("%d:fileName=%s\n",fileCount,tmpFile.name());
        tmpFile.close();
    }
    printf("fileCount=%d\n",fileCount);
    currentDir.rewindDirectory();
}

void newFile(char *fName){//fName=RecPlay1.mp3
    char tmpName[100];
    strcpy(tmpName,REC_ROOT_PATH);//tmpName="RECORD"
    strcat(tmpName,"/");//tmpName="RECORD/"
    strcat(tmpName,fName);//tmpName="RECORD/RecPlay1.mp3"
    
    if(theSD.exists(tmpName)){//如果同名文件已经存在，删除原文件。加上此if语句，录音时，就不文件追加了。
      theSD.remove(tmpName);
   }
   
    currentFile = theSD.open(tmpName, FILE_WRITE);//  mnt/sd0/RECORD/RecPlay1.mp3
    /* Verify file open */
    if (!currentFile)
    {
        printf("File open error\n");
        exit(1);
    }
    printf("success new file! filename=%s\n", currentFile.name());
}

void openFile(char *fName){//fName=RecPlay1.mp3
    currentDir.rewindDirectory();
    for(int i =0; i < fileCount; i ++){
        currentFile = currentDir.openNextFile();// /mnt/sd0/RECORD/RecPlay1.mp3
        if(strstr(currentFile.name(),fName)==NULL){
            currentFile.close();
        }else{//找到
            break;
        }
    }
    /* Verify file open */
    if (!currentFile)
    {
        printf("File open error\n");
        exit(1);
    } 
    printf("success open, currentFile=%s\n",currentFile.name());
}

void initAudio(){
    // start audio system
    theAudio = AudioClass::getInstance();

    theAudio->begin();

    puts("initialization Audio Library");

    /* Set output device to speaker */
    theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
    theAudio->setVolume(DB_value);
}   


/**
 * @brief Audio player set up and start
 *
 * Set audio player to play mp3/Stereo audio.
 * Audio data is read from SD card.
 *
 */
void playerMode()
{
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP);

  /*
   * Set main player to decode stereo mp3. Stream sample rate is set to "auto detect"
   * Search for MP3 decoder in "/mnt/sd0/BIN" directory
   */
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("Player0 initialize error\n");
      exit(1);
    }

  /* Send first frames to be decoded */
  err = theAudio->writeFrames(AudioClass::Player0, currentFile);

  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("File Read Error! =%d\n",err);
      currentFile.close();
      exit(1);
    }

  printf("Open file! filename=%s\n", currentFile.name());
  printf("Begin to Play file!\n" );

//  /* Main volume set to -16.0 dB, Main player and sub player set to 0 dB */
//  theAudio->setVolume(DB_value, 0, 0);
  theAudio->startPlayer(AudioClass::Player0);
}

/**
 * @brief Supply audio data to the buffer
 */
bool playStream()
{
  /* Send new frames to decode in a loop until file ends */
  err_t err = theAudio->writeFrames(AudioClass::Player0, currentFile);

  /*  Tell when player file ends */
  if (err == AUDIOLIB_ECODE_FILEEND)
    {
      printf("Play this file End!\n ");
      goto stop_player;
    }

  /* Show error code from player and stop */
  if (err && err != AUDIOLIB_ECODE_FILEEND)//error occurd
    {
      printf("Main player error code: %d\n", err);
      goto stop_player;
    }

  usleep(100000);//挂起
  printf("Play file......\n" );
  
  /* Don't go further and continue play */
  return false;

stop_player:
  sleep(1);
  theAudio->stopPlayer(AudioClass::Player0);
  currentFile.close();

  /* return play end */
  return true;
}


/**
 * @brief Audio recorder set up and start
 *
 * Set audio recorder to record mp3/Stereo audio.
 * Audio data is written to SD card.
 *
 */
void recorderMode()
{
  /* Select input device as analog microphone */
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);

  /*
   * Initialize filetype to stereo mp3 with 48 Kb/s sampling rate
   * Search for MP3 codec in "/mnt/sd0/BIN" directory
   */
  theAudio->initRecorder(AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_48000, AS_CHANNEL_STEREO);


  /* Open file for data write on SD card */
  
  printf("Begin to Rec file\n" );

  theAudio->startRecorder();
}

/**
 * @brief Read audio data from the buffer
 */
bool recordStream()
{
  static int cnt = 0;
  err_t err = AUDIOLIB_ECODE_OK;

  /* recording end condition */
  if (cnt > RecordLoopNum)
    {
      usleep(1000);
      puts("End Recording!!");
      goto stop_recorder;
    }

  /* Read frames to record in file */
  err = theAudio->readFrames(currentFile);

  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("File error End! record error =%d\n",err);
      goto stop_recorder;
    }
    
  printf("Rec file......\n" );
  usleep(20000);
  cnt++;

  /* return still recording */
  return false;

stop_recorder:
  sleep(1);
  theAudio->stopRecorder();
  theAudio->closeOutputFile(currentFile);
  currentFile.close();
  cnt = 0;

  /* return record end */
  return true;
}

/**
 * @brief Rec and play streams
 *
 * Repeat recording and playing stream files
 */
void loop()
{
    char tmpName[100];
    static int s_state = None;
    // puts("loop!!");
    char currentRecFileName[16]="RecPlay";
    char currentPlayFileName[16]="RecPlay";
    String cmdStr;
    if(Serial.available()>0){
        cmdStr=Serial.readString();
        Serial.println(cmdStr);
        if(cmdStr.startsWith("record_new")){//录音
            printf("record_new\n");
            s_state = RecordReady;
        }else if(cmdStr.startsWith("play_new")){//播放
            printf("play_new\n");
            s_state = PlayReady;
        }else if(cmdStr.startsWith("remove_new")){//删除
            printf("remove_new\n");
            s_state = Remove;
        }else if(cmdStr.startsWith("ChoseRemove_")){//选择删除
            cmdStr.remove(0,12);
            printf("remove:%s\n",cmdStr);
            s_state = ChoseRemove;
        }else if(cmdStr.startsWith("ChosePlay_")){//选择播放
            cmdStr.remove(0,12);
            printf("play:%s\n",cmdStr.c_str());
            s_state = ChosePlay;
        }
    }
    switch (s_state)
    {
      case PlayReady:
        snprintf(currentPlayFileName, sizeof(currentPlayFileName), "RecPlay%d.mp3", fileCount);
        openFile(currentPlayFileName);
        playerMode();
        s_state = Playing;
        break;
        
      case Playing:
        if (playStream())
          {
            theAudio->setReadyMode();
            s_state = None;
          }
        break;
        
      case RecordReady:
        fileCount++;
        snprintf(currentRecFileName, sizeof(currentRecFileName), "RecPlay%d.mp3", fileCount);
        newFile(currentRecFileName);//新建文件
        recorderMode();
        s_state = Recording;
        break;
        
      case Recording:
        if (recordStream())
          {
            theAudio->setReadyMode();
            s_state = None;
          }
        break;

       case Remove:
            strcpy(tmpName,REC_ROOT_PATH);//tmpName="RECORD"
            strcat(tmpName,"/");//tmpName="RECORD/"
            snprintf(currentRecFileName, sizeof(currentPlayFileName), "RecPlay%d.mp3", fileCount);
            strcat(tmpName,currentRecFileName);//tmpName="RECORD/RecPlay1.mp3"
            theSD.remove(tmpName);
            fileCount--;
            printf("success remove,currentFileName=%s\n",tmpName);
             s_state = None;
         break;

         case ChoseRemove:
            strcpy(tmpName,REC_ROOT_PATH);//tmpName="RECORD"
            strcat(tmpName,"/");//tmpName="RECORD/"
            strcat(tmpName,cmdStr.c_str());//tmpName="RECORD/RecPlay1.mp3"
            theSD.remove(tmpName);
            fileCount--;
            printf("success remove,currentFileName=%s\n",tmpName);
             s_state = None;
         break;

         case ChosePlay:
            snprintf(currentPlayFileName, sizeof(currentPlayFileName), "%s",cmdStr.c_str());
            openFile(currentPlayFileName);
            playerMode();
            s_state = Playing;
         break;
            
      default:
        break;
    }
}
