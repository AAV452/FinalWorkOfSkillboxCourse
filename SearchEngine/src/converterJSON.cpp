#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp> //подключение библиотеки из папки "nlohmann_json" в проекте см. CMakeLists.txt
#include "converterJSON.h" //описание класса работы с JSON


/*
    Сериализация — это процесс преобразования объекта в формат (например, последовательность байтов, XML или JSON),
    который может быть сохранён или передан, например, по сети или в файл. Обратный процесс - восстановление 
    объекта из этого формата - называется десериализацией. Основная цель сериализации — сохранить состояние объекта 
    для последующего воссоздания. 
*/
//метод десериализации файлов config.json и request.json. Необходим, если пользователь захочет скорректировать
//неправильные json-файлы и дессериализовать их снова в настройки класса во время работы программы поисковика.
//вернёт true, если json-файлы правильно оформлены, иначе false:
bool ConverterJSON::initialisationEngineFromJson()
{
  bool result = true; //по умолчанию будем считать, что json-файлы верно заполнены
  
  //задание пути к папке с json-файлами:
  std::filesystem::path p = "";
  p /= "..\\..\\json\\config.json"; // /= - правильно соединяет пути

  std::ifstream ifile(p);

  if ( !ifile.is_open() )
  {
    std::cout << "Отсутствует файл конфигурации (config file is missing)";
    result = false;
  }
  else
  {
    nlohmann::json dictJSON; //создание объекта-словаря JSON
    //десериализация JSON:
    ifile >> dictJSON; //внутренний курсор дошёл до конца файла

    //ставим внутренний курсор на начало файла (задаём смещение на 0 символов от начала):
    ifile.seekg(0, std::ios::beg);

    //вывод файла в стиле JSON:
    //std::cout << dictJSON.dump(4) << std::endl; //dump() - JSON в строку, 4 - отступ в 4 пробела
  
    //парсим JSON:
    //для корректного извлечения данных из JSON необходимо обрабатывать возможную ситуацию на отсутствие полей или значений. 
    //1. способ: проверка через метод contains():
    //    if ( j.contains("field_name") ) auto value = j["field_name"];    else {//обработка отсутствия поля}
    //2. способ: безопасное получение value():
    //    std::string s = j.value("field_name", "default"); //вернёт "default", если поля нет
    //3. способ: методом find():
    //    auto it = j.find("key");
    //    if ( it != j.end() )  std::string value = it.value(); //значение найдено
    //4. способ: обработка исключения (try-catch):
    //    try { auto value = j.at("field_name"); } //at() кидает исключение
    //      catch (nlohmann::json::out_of_range& e) { std::cerr << "Поле не найдено: " << e.what() << std::endl; }
    //использование contains() или value() предпочтительнее, т.к. исключения более затратны по производительности:

    try 
    {
//******************** ДЕСЕРИАЛИЗАЦИЯ КОНФИГУРАЦИИ ПОИСКОВОГО ДВИЖКА ***********************
      dictJSON.at("config"); //если поля нет - будет брошено исключение nlohmann::json::out_of_range

      if ( dictJSON["config"].contains("name") ) 
        config.name = dictJSON["config"]["name"];
      else
      {
        std::cout << "Название поискового движка отсутствует. Поэтому будет задано \"unknown engine\"\n";
        config.name = "unknown engine";
      }

      //config.version = dictJSON["config"]["version"];
      config.version = dictJSON["config"].value("version", "не определена"); //вернёт "не определена", если поля нет
      //config.max_responses = dictJSON["config"]["max_responses"];
      config.max_responses = dictJSON["config"].value("max_responses", 5); //вернёт 5, если поля нет

      //покажем пользователю настройки поисковика:
      showConfigEngine(); //метод класса

//************************* ДЕСЕРИАЛИЗАЦИЯ СПИСКА ФАЙЛОВ ДЛЯ ПОИСКА *****************************
      //предварительно очищаем вектор путей к файлам для поиска:
      config.filesPaths.clear();
      //теперь очищаем память от вектора:
      //гарантированно освобождаем память временным пустым вектором (для версий до С++11)
      //std::vector<std::string>().swap(textsFromFilesVec);
      //запрос на уменьшение ёмкости до текущего размера (в данном случае 0) (доступно с С++11)
      config.filesPaths.shrink_to_fit();

      //заполнение списка файлов для поиска:
      auto it = dictJSON.find("files"); //если поле не найдено - итератор укажет на end.
      if ( it == dictJSON.end() )
      {
        std::cout << "В файле config.json нет поля \"files\"\n";
        throw std::exception(); //вызов исключения и переход в catch для его обработки
      }
      else
      if ( it.value().empty() )
      {
        std::cout << "Нет ни одного файла для поиска\n";
        //для вывода пользовательского исключения нужно перегрузить метод what() в классе exception
        //надо бы сделать!!!!!!!!!!!!!!!!!!!!! ПОТОМ!!!!!!!!!!!!!!!!!!!
        throw std::exception(); //вызов исключения и переход в catch для его обработки
      }
      else
        for ( auto i : it.value() )
          config.filesPaths.push_back( i.get<std::string>() ); //добавляем поле JSON в список путей к файлам для поиска


//************************* ДЕСЕРИАЛИЗАЦИЯ requests.json ФАЙЛА ПОИСКОВЫХ ЗАПРОСОВ *****************************
      requestsFromJsonToVec(); //заполнили в классе вектор запросов 
    }
    catch ( const nlohmann::json::exception& e )
    {
      config.filesPaths.clear(); //очищаем список файлов для поиска
      config.filesPaths.shrink_to_fit(); //запрос на уменьшение ёмкости до текущего размера (в данном случае 0) (доступно с С++11)
      std::cerr << "Ошибка JSON: " << e.what() << ". Файл конфигурации пуст (config file is empty)\n";
      result = false;
    }
    catch ( std::exception& e )
    {
      config.filesPaths.clear(); //очищаем список файлов для поиска
      config.filesPaths.shrink_to_fit(); //запрос на уменьшение ёмкости до текущего размера (в данном случае 0) (доступно с С++11)
      std::cerr << "Ошибка JSON: " << e.what() << ". Файл конфигурации испорчен\n";
      result = false;
    }
  }    
  ifile.close();

  return result;
}

//метод вывода настроек поисковика:
void ConverterJSON::showConfigEngine()
{
  std::cout << "\nЗапущен поисковик \"" << config.name << "\", версия " << config.version
                << ", макс. количество ответов на запрос " << config.max_responses << std::endl << std::endl;
}

/*
//метод получения содержимого файлов возвращает список с содержимым файлов,
//перечисленных в config.json:
std::vector<std::string> ConverterJSON::getTextDocuments()
{
  return textsFromFilesVec; //этот вектор заполняется на этапе конфигурации
}
*/


//метод подсчитывания поля max_responses для определения предельного количества
//ответов на один запрос:
int ConverterJSON::getResponsesLimit()
{
  return config.max_responses;
}


//метод десериализации файла requests.json в поле класса список запросов:
void ConverterJSON::requestsFromJsonToVec()
{
  //задание пути к папке с json-файлами:
  std::filesystem::path p = "";
  p /= "..\\..\\json\\requests.json"; // /= - правильно соединяет пути

  std::ifstream ifile(p);

  if ( !ifile.is_open() )
    std::cout << "Отсутствует файл запросов (requests file is missing)";
  else
  {
    //десериализация JSON:
    nlohmann::json dictJSON; //создание объекта-словаря JSON
    
    ifile >> dictJSON; //внутренний курсор дошёл до конца файла

    //ставим внутренний курсор на начало файла (задаём смещение на 0 символов от начала):
    ifile.seekg(0, std::ios::beg);

    try 
    {
      dictJSON.at("requests"); //если поля нет - будет брошено исключение nlohmann::json::out_of_range

      auto it = dictJSON.find("requests");
      if ( it != dictJSON.end() )
      {
        if ( it.value().empty() )
        {
          std::cout << "Нет ни одного запроса для поиска\n";
          //для вывода пользовательского исключения нужно перегрузить метод what() в классе exception
          //надо бы сделать!!!!!!!!!!!!!!!!!!!!! ПОТОМ!!!!!!!!!!!!!!!!!!!
          throw std::exception(); //вызов исключения и переход в catch для его обработки
        }
        else
        {
          //десериализация JSON-массива:
          int maxRequestCount = 0;
          /*//проход по массиву по-старинке:
          for ( nlohmann::json::iterator it = dictJSON["requests"].begin(); it != dictJSON["requests"].end(); it++ )
            requestsVec.push_back(it.value().get<std::string>());
          */
          //проход по массиву, начиная с С++11:
          for ( auto i : it.value() )
          {
            auto str = i.get<std::string>(); //поле JSON представляем строкой

            if ( maxRequestCount <= 1000 ) //по заданию - не более 1000
            {
              //std::cout << str << std::endl;
              requestsVec.push_back(str);
              maxRequestCount++;
            }
            else
            {
              std::cout << "Количество запросов превысило 1000. Лишние запросы обрабатываться не будут!\n";
              break; //for
            }
          }
        }
      }
    }
    catch ( const nlohmann::json::exception& e )
    {
      requestsVec.clear(); //очищаем список запросов
      requestsVec.shrink_to_fit(); //запрос на уменьшение ёмкости до текущего размера (в данном случае 0) (доступно с С++11)
      std::cerr << "Ошибка JSON: " << e.what() << ".\nФайл запросов пуст (requests file is empty)\n";
    }
    catch ( std::exception& e )
    {
      requestsVec.clear(); //очищаем список запросов
      requestsVec.shrink_to_fit(); //запрос на уменьшение ёмкости до текущего размера (в данном случае 0) (доступно с С++11)
      std::cerr << "Ошибка JSON: " << e.what() << ".\nФайл запросов испорчен\n";
    }
  }

  ifile.close();
}

//метод вывода путей ко всем файлам для поиска:
std::vector<std::string> ConverterJSON::getDocumentPaths()
{
  return config.filesPaths;
}

//метод получения запросов из файла requests.json
//возвращает список запросов из файла requests.json:
std::vector<std::string> ConverterJSON::getRequests()
{
  return requestsVec;
}

//положить в файл answers.json результаты поисковых запросов:
void ConverterJSON::putAnswers( std::vector<std::vector<std::pair<int, float>>> &answers )
{
  //задание пути к папке с json-файлами:
  std::filesystem::path p = "";
  p /= "..\\..\\json\\answers.json"; // /= - правильно соединяет пути

  //если нужно открыть файл для дозаписи в конец (append):     std::ofstream ofile("file.txt", std::ios::app);
  //если для чтения и записи поверх старого (изменить что-то): std::ofstream ofile("file.txt", std::ios::in | std::ios::out);
  //по умолчанию файл будет перезаписан (старое содержимое удалится, а запись начнётся с нуля):
  std::ofstream ofile(p);

  /*
  Пример содержимого файла answers.json:
        {
          "answers": {
              "request001": {
                  "result": "true",
                  "relevace": [
                      {"docid": 0, "rank": 0.989},
                      {"docid": 1, "rank": 0.897},
                      {"docid": 2, "rank": 0.750}
                  ]
              },
              "request002": {
                  "result": "true",
                  "relevace": [
                      {"docid": 0, "rank": 0.769}
                  ]
              },
              "request003": {
                  "result": "false"
              }
          }
        }
  */
  
  nlohmann::json j; //создание объекта-словаря JSON

  for ( int i = 0; i < answers.size(); i++ )
  {
    std::string requestN = "request00" + std::to_string(i+1);

    if ( answers[i].empty() )
      j["answers"][requestN]["rezult"] = false;
    else
    {
      j["answers"][requestN]["rezult"] = true;

      for ( int n = 0; n < answers[i].size(); n++ )
      {
        j["answers"][requestN]["relevace"][n]["doc_id"] = answers[i][n].first;
        j["answers"][requestN]["relevace"][n]["rank"]   = answers[i][n].second;
      }
    }
  }
  
  //std::cout << j.dump(4) << std::endl;

  ofile << j.dump(4); //задаём тип отображения в стиле JSON с отступами

  ofile.close();
}