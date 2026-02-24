#pragma once //прагма - команда прекомпилятора - исключает повторение и пересечение вызовов
             //данного .h-файла в других срр-файлах и разных сценариях проекта


struct Config
{
  std::string name = "";
  std::string version = "";
  int max_responses = 5;
  std::vector<std::string> filesPaths;
};

//Класс для работы с JSON-файлами:
class ConverterJSON
{
private:

  Config config; //поле с параметрами программы, взятыми из config.json
  //std::vector<std::string> textsFromFilesVec; //список содержимого по каждому найденному файлу (ОПИСАН В КЛАССЕ ИНДЕКСАЦИИ)
  std::vector<std::string> requestsVec; //список запросов из файла requests.json
  
public:

  ConverterJSON() = default;
  
  ~ConverterJSON() { /*std::cout << "Класс ConverterJSON отработал!!!";*/ };

  //метод десериализации файлов config.json и request.json. Необходим, если пользователь захочет скорректировать
  //неправильные json-файлы и дессериализовать их снова в настройки класса во время работы программы поисковика.
  //вернёт true, если json-файлы правильно оформлены, иначе false:
  bool initialisationEngineFromJson();

  //метод вывода настроек поисковика:
  void showConfigEngine();

  /* ТЕКСТЫ ПОЛУЧИМ В КЛАССЕ ИНДЕКСАЦИИ!!!!
  //метод получения содержимого файлов возвращает список с содержимым файлов,
  //перечисленных в config.json:
  std::vector<std::string> getTextDocuments();
  */

  //метод вывода путей ко всем файлам для поиска:
  std::vector<std::string> getDocumentPaths();

  //метод подсчитывания поля max_responses для определения предельного количества
  //ответов на один запрос:
  int getResponsesLimit();

  //метод десериализации файла requests.json в поле класса список запросов:
  void requestsFromJsonToVec();

  //метод получения запросов из файла requests.json
  //возвращает список запросов из файла requests.json:
  std::vector<std::string> getRequests();

  //положить в файл answers.json результаты поисковых запросов:
  void putAnswers( std::vector<std::vector<std::pair<int, float>>> &answers );
};