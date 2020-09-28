#include <Audio.h>

#define STAT_PLAY 0
#define STAT_STOP 1
#define MUSIC_ROOT_PATH "Music"
#define RecordLoopNum 200
#define REC_ROOT_PATH "RECORD"

enum
{
  PlayReady = 0,
  Playing,
  RecordReady,
  Recording,
  None,//空闲状态
  Remove,
  ChoseRemove,
  ChosePlay
};

SDClass theSD;
AudioClass *theAudio;

File currentFile;
File currentDir;

int fileCount = 0;
int currentPosition = 0;

int DB_volume = -160;
bool playerFlag = true;//播放器/录音器切换功能标志位
bool playFlag = false;//播放音乐标志位
bool stopFlag = false;//停止标志位
bool endFlag = true;//播放结束标志位
bool pluFlag = false;//音量加标志位
bool subFlag = false;//音量减标志位
bool singleFlag = true; //单曲循环标志位
bool autoFlag = false;//循环播放标志位
bool randomModelFlag = false;//随机播放标志位

char currentStat = STAT_STOP;

/**
   @brief Set up audio library to record and play

   Get instance of audio library and begin.
   Set rendering clock to NORMAL(48kHz).
*/
void setup()
{
  Serial.begin(115200);
  Serial.println("Serial init OK\n");
  initAudio();
  init_SD();
}

void init_SD() {
  if (playerFlag) {
    currentDir = theSD.open(MUSIC_ROOT_PATH);
  } else {
    currentDir = theSD.open(REC_ROOT_PATH);
  }
  printf("currentDir=%s\n", currentDir.name());
  if (!currentDir) {
    printf("Err: dir not exits\n");
  } else {
    printf("dir open ok\n");
  }
  countFile();
}

void countFile() {
  File tmpFile;
  while ((tmpFile = currentDir.openNextFile()) != NULL) {
    fileCount++;
    printf("%d: fileName=%s\n", fileCount, tmpFile.name());
    tmpFile.close();
  }
  printf("fileCount = %d\n", fileCount);
  currentDir.rewindDirectory();
  currentPosition = 0;
}

/**
  @brief Audio player set up and start

  Set audio player to play mp3/Stereo audio.
  Audio data is read from SD card.

*/
void playerMode()
{
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP);
  /*
     Set main player to decode stereo mp3. Stream sample rate is set to "auto detect"
     Search for MP3 decoder in "/mnt/sd0/BIN" directory
  */
 err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
  {
    printf("Player0 initialize error\n");
    exit(1);
  }

  /* Send first frames to be decoded */
  err = theAudio->writeFrames(AudioClass::Player0,currentFile);

  if (err != AUDIOLIB_ECODE_OK)
  {
    printf("File Read Error! =%d\n", err);
    currentFile.close();
    exit(1);
  }

  printf("Open file! filename=%s\n", currentFile.name());
  printf("Begin to Play file!\n" );
  theAudio->startPlayer(AudioClass::Player0);

}

void initAudio() { //对Audio初始化
  // start audio system
  theAudio = AudioClass::getInstance();

  theAudio->begin();

  puts("initialization Audio Library");

  /* Set output device to speaker */
  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
  theAudio->setVolume(DB_volume);//设置音量
}

/**
   @brief Supply audio data to the buffer
*/
bool playStream()//持续播放音乐
{
  /* Send new frames to decode in a loop until file ends */
  printf("currentFile position = %d\n available = %d\n currentFileName = %s\n", currentFile.position(), currentFile.available(), currentFile.name());
  err_t err = theAudio->writeFrames(AudioClass::Player0, currentFile);

  /*  Tell when player file ends */
  if (err == AUDIOLIB_ECODE_FILEEND)
  {
    printf("Main player File End!\n");
    endFlag = true;
    goto stop_player;
  }

  /* Show error code from player and stop */
  if (err && err != AUDIOLIB_ECODE_FILEEND)
  {
    printf("Main player error code: %d\n", err);
    goto stop_player;
  }

  usleep(40000);

  /* Don't go further and continue play */
  return false;

stop_player:
  sleep(1);
  theAudio->stopPlayer(AudioClass::Player0);
  currentFile.close();
  /* return play end */
  return true;
}

void audioStop() {
  printf("the audio is going to stop");
  theAudio->stopPlayer(AudioClass::Player0);
}



/***************************************/
void getNextFile() {
  if (currentPosition >= 1) {
    currentFile.close();
  } else {
    printf("this is the first File!/n");
  }
  currentFile = currentDir.openNextFile();
  if (currentFile == NULL) {
    currentDir.rewindDirectory();
    currentFile = currentDir.openNextFile();//打开了第一个文件
    currentPosition = 1;
  } else {
    ++currentPosition;
  }
  printf("currentFile position = %d\n available = %d\n currentFileName = %s\n", currentFile.position(), currentFile.available(), currentFile.name());

}

void getPreFile() {
  File tmpFile;
  currentPosition--;
  if (currentPosition >= 1) {
    currentFile.close();
    currentDir.rewindDirectory();
    for (int i = 0; i < currentPosition - 1; i++) {
      tmpFile = currentDir.openNextFile();
      tmpFile.close();
    }
    currentFile = currentDir.openNextFile();
    printf("currentFile position = %d\n available = %d\n currentFileName = %s\n", currentFile.position(), currentFile.available(), currentFile.name());
  } else {
    printf("Err:no premusic!\n");
    ++currentPosition;
  }
}

void getRandomFile() {
  File tmpFile;
  int randomNum = random(1, fileCount + 1);
  if (currentPosition >= 1) {
    currentFile.close();
  } else {
    printf("this is the first File!/n");
  }
  currentDir.rewindDirectory();
  currentPosition = randomNum;
  while (--randomNum) {
    tmpFile = currentDir.openNextFile();
    tmpFile.close();
  }
  
  currentFile = currentDir.openNextFile();
  printf("currentFile position = %d\n available = %d\n currentFileName = %s\n", currentFile.position(), currentFile.available(), currentFile.name());
}

void fastForward() {
  audioStop();
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
  /* Send first frames to be decoded */
  err = theAudio->writeFrames(AudioClass::Player0, currentFile);
  if (err != AUDIOLIB_ECODE_OK)
  {
    printf("File Read Error! =%d\n", err);
    currentFile.close();
    exit(1);
  }
  playerMode();
}

void chosePlayMusic(char* fName) {
  if (currentPosition >= 1) {
    currentFile.close();
  }
  currentDir.rewindDirectory();
  for (int i = 0; i < fileCount; i++) {
    currentFile = currentDir.openNextFile();// /mnt/sd0/RECORD/RecPlay1.mp3
    if (strstr(currentFile.name(), fName) == NULL) {
      currentFile.close();
    } else { //找到
      currentPosition = i + 1;
      break;
    }
  }
}


/**
   @brief Audio recorder set up and start

   Set audio recorder to record mp3/Stereo audio.
   Audio data is written to SD card.

*/
void newFile(char *fName) { //fName=RecPlay1.mp3
  char tmpName[100];
  strcpy(tmpName, REC_ROOT_PATH); //tmpName="RECORD"
  strcat(tmpName, "/"); //tmpName="RECORD/"
  strcat(tmpName, fName); //tmpName="RECORD/RecPlay1.mp3"
  if (theSD.exists(tmpName)) { //如果同名文件已经存在，删除原文件。加上此if语句，录音时，就不文件追加了。
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

void openFile(char *fName) { //fName=RecPlay1.mp3
  currentDir.rewindDirectory();
  for (int i = 0; i < fileCount; i ++) {
    currentFile = currentDir.openNextFile();// /mnt/sd0/RECORD/RecPlay1.mp3
    if (strstr(currentFile.name(), fName) == NULL) {
      currentFile.close();
    } else { //找到
      break;
    }
  }
  /* Verify file open */
  if (!currentFile)
  {
    printf("File open error\n");
    exit(1);
  }else{
      printf("success open, currentFile=%s\n", currentFile.name());
    }
}

/**
   @brief Audio recorder set up and start

   Set audio recorder to record mp3/Stereo audio.
   Audio data is written to SD card.

*/
void recorderMode()
{
  /* Select input device as analog microphone */
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC);

  /*
     Initialize filetype to stereo mp3 with 48 Kb/s sampling rate
     Search for MP3 codec in "/mnt/sd0/BIN" directory
  */
  theAudio->initRecorder(AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_48000, AS_CHANNEL_STEREO);


  /* Open file for data write on SD card */

  printf("Begin to Rec file\n" );

  theAudio->startRecorder();
}

/**
   @brief Read audio data from the buffer
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
    printf("File error End! record error =%d\n", err);
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
   @brief Rec and play streams

   Repeat recording and playing stream files
*/
void loop()
{
  char tmpName[100];
  static int s_state = None;
  char currentRecFileName[16] = "RecPlay";
  char currentPlayFileName[16] = "RecPlay";
  String cmdStr;
  if (Serial.available() > 0) {
    cmdStr = Serial.readString();
    Serial.println(cmdStr);
    if (cmdStr.startsWith("player") && s_state == None) { //选择音乐播放器功能
      playerFlag = true;
      currentFile.close();
      fileCount = 0;
      init_SD();
      printf("This is player function!\n");
      getNextFile();
    } else if (cmdStr.startsWith("recorder") && currentStat == STAT_STOP) { //选择录音机功能
      playerFlag = false;
      currentFile.close();
      theAudio->setReadyMode();
      fileCount = 0;
      init_SD();
      printf("This is recorder function!\n");
    } else if (cmdStr.startsWith("start") && playerFlag) { //开始播放音乐
      printf("cmd= start\n");
      playFlag = true;
    } else if (cmdStr.startsWith("stop") && playerFlag) { //停止播放音乐
      printf("cmd= stop\n");
      stopFlag = true;
    } else if (cmdStr.startsWith("singleModel") && playerFlag) { //单曲循环模式
      printf("cmd= singleModel\n");
      singleFlag = true;
      autoFlag = false;
      randomModelFlag = false;
    } else if (cmdStr.startsWith("autoModel") && playerFlag) { //循环播放模式
      printf("cmd= autoModel\n");
      singleFlag = false;
      autoFlag = true;
      randomModelFlag = false;
    } else if (cmdStr.startsWith("randomModel") && playerFlag) { //随机播放模式
      printf("cmd= randomModel\n");
      singleFlag = false;
      autoFlag = false;
      randomModelFlag = true;
    } else if (cmdStr.startsWith("next") && playerFlag) { //播放下一首歌
      getNextFile();
    } else if (cmdStr.startsWith("pre") && playerFlag) { //播放上一首歌
      getPreFile();
    } else if (cmdStr.startsWith("random") && playerFlag) { //随机播放下一首
      getRandomFile();
    } else if (cmdStr.startsWith("ChosePlayMusic_") && playerFlag) { //选择播放某一首歌
      cmdStr.remove(0, 17);
      strcpy(tmpName, cmdStr.c_str());
      chosePlayMusic(tmpName);
    } else if (cmdStr.startsWith("fastForward") && currentStat == STAT_PLAY && playerFlag) { //快进
      printf("cmd=fastForward\n");
      printf("FastForward.......\n");
      fastForward();
    } else if (cmdStr.startsWith("record_new")) { //录音
      printf("record_new\n");
      s_state = RecordReady;
    } else if (cmdStr.startsWith("play_new")) { //播放
      printf("play_new\n");
      s_state = PlayReady;
    } else if (cmdStr.startsWith("remove_new")) { //删除
      printf("remove_new\n");
      s_state = Remove;
    } else if (cmdStr.startsWith("ChoseRemove_")) { //选择删除
      cmdStr.remove(0, 12);
      printf("remove:%s\n", cmdStr.c_str());
      s_state = ChoseRemove;
    } else if (cmdStr.startsWith("ChosePlay_")) { //选择播放
      cmdStr.remove(0, 12);
      printf("play:%s\n", cmdStr.c_str());
      s_state = ChosePlay;
    } else if (cmdStr.startsWith("+")) { //音量加
      printf("cmd= +\n");
      pluFlag = true;
    } else if (cmdStr.startsWith("-")) { //音量减
      printf("cmd= -\n");
      subFlag = true;
    }
  }

  /************************************/
  if (playFlag) {
    playFlag = false;
    if (currentStat == STAT_STOP) {
      currentStat = STAT_PLAY;
      playerMode();//启动音乐播放器
    }
  }
  if (stopFlag) {
    stopFlag = false;
    if (currentStat == STAT_PLAY) {
      audioStop();
      currentStat = STAT_STOP;
    }
  }

  if (!endFlag && currentStat == STAT_PLAY) {
    playStream();
  }

  if (pluFlag) {
    pluFlag = false;
    DB_volume += 50;
    theAudio->setVolume(DB_volume);//设置音量
  }
  if (subFlag) {
    subFlag = false;
    DB_volume -= 50;
    theAudio->setVolume(DB_volume);//设置音量
  }

  if (singleFlag && playerFlag) {
    File tmpFile;
    if (endFlag) {
      endFlag = false;
      if (currentPosition < 1) {
        currentFile = currentDir.openNextFile();
        ++currentPosition;
      } else {
        currentDir.rewindDirectory();
        currentFile.close();
        for (int i = 0; i < currentPosition - 1; i++) {
          tmpFile = currentDir.openNextFile();
          tmpFile.close();
        }
        currentFile = currentDir.openNextFile();
      }
      if (currentStat == STAT_PLAY) {
        playerMode();//启动音乐播放器
      }
      printf("currentFile position = %d\n available = %d\n currentFileName = %s\n", currentFile.position(), currentFile.available(), currentFile.name());
    }
  }

  if (autoFlag && playerFlag) {
    if (endFlag) {
      endFlag = false;
      getNextFile();
      if (currentStat == STAT_PLAY) {
        playerMode();//启动音乐播放器
      }
      printf("currentFile position = %d\n available = %d\n currentFileName = %s\n", currentFile.position(), currentFile.available(), currentFile.name());
    }
  }

  if (randomModelFlag && playerFlag) {
    if (endFlag) {
      endFlag = false;
      getRandomFile();
      if (currentStat == STAT_PLAY) {
        playerMode();//启动音乐播放器
      }
      printf("currentFile position = %d\n available = %d\n currentFileName = %s\n", currentFile.position(), currentFile.available(), currentFile.name());
    }
  }
  /********************************************************/
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
      strcpy(tmpName, REC_ROOT_PATH); //tmpName="RECORD"
      strcat(tmpName, "/"); //tmpName="RECORD/"
      snprintf(currentRecFileName, sizeof(currentPlayFileName), "RecPlay%d.mp3", fileCount);
      strcat(tmpName, currentRecFileName); //tmpName="RECORD/RecPlay1.mp3"
      theSD.remove(tmpName);
      fileCount--;
      printf("success remove,currentFileName=%s\n", tmpName);
      s_state = None;
      break;

    case ChoseRemove:
      strcpy(tmpName, REC_ROOT_PATH); //tmpName="RECORD"
      strcat(tmpName, "/"); //tmpName="RECORD/"
      strcat(tmpName, cmdStr.c_str()); //tmpName="RECORD/RecPlay1.mp3"
      theSD.remove(tmpName);
      fileCount--;
      printf("success remove,currentFileName=%s\n", tmpName);
      s_state = None;
      break;

    case ChosePlay:
      snprintf(currentPlayFileName, sizeof(currentPlayFileName), "%s", cmdStr.c_str());
      openFile(currentPlayFileName);
      if (currentFile) {
        playerMode();
        s_state = Playing;
      } else {
        printf("Err:open Failed!\n");
      }
      break;

    default:
      break;
  }
}
