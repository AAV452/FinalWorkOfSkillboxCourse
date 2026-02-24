#include <iostream>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp> //подключение библиотеки из папки "nlohmann_json" в проекте см. CMakeLists.txt
#include "converterJSON.h" //описание класса работы с JSON
#include "invertedIndex.h"
#include "searchServer.h"

//функция вывода в консоль результата итогового расчёта релевантности всех документов всем запросам:
void showRelevanceRezult(std::vector<std::vector<RelativeIndex>> inputVec)
{
  if ( inputVec.size() == 1 )
    std::cout << "\nРелевантность документов запросу:\n";
  else
    std::cout << "\nРелевантность документов запросам:\n";
        
  for ( auto serSerOut : inputVec )
  {
    std::cout << "\t";
    if ( serSerOut.empty() )
      std::cout << "{}";
    else
    for ( auto vec : serSerOut )
      std::cout << "{doc_id = " << vec.doc_id << ", rank = " << vec.rank << "} ";
            
    std::cout << std::endl;
  }
}


int main()
{
  ConverterJSON *converterJSON = new ConverterJSON();

  //десериализуем config.json и requests.json (заполняем поисковик данными):
  if ( !converterJSON->initialisationEngineFromJson() )
  {
    delete converterJSON;
    std::cout << "Исправьте файл \"config.json\" и повторите запуск программы!" << std::endl;
    return 0;
  }

  InvertedIndex *invertedIndex = new InvertedIndex();
  //запускаем индексацию файлов (получаем содерживое валидных файлов, разбираем на слова,
  //создаём и заполняем частотный словарь):
  invertedIndex->updateDocumentsBase(converterJSON->getDocumentPaths());

  SearchServer *searchServer = new SearchServer(*invertedIndex);

  std::string command = "";
  while ( true )
  {
    std::cout << "-1 - exit;\n"
              << " 1 - Обновить базу (заново запустить индексацию файлов из \"resources\\\", если они были изменены);\n"
              << " 2 - Показать содержимое валидных файлов;\n"
              << " 3 - Показать частотный словарь типа 'слово {индекс_файла, количество_слова_в_файле}';\n"
              << " 4 - Показать релевантность 'запросу вручную';\n"
              << " 5 - Показать запросы из \"json\\requests.json\";\n"
              << " 6 - Показать релевантность запросам из \"json\\requests.json\" и\n"
              << "     сериализовать ответ в \"json\\answers.json\".\n"
              << "\nВведите номер команды: ";
    std::cin >> command;

    if ( command == "-1" )  break;
    else
    if ( command == "1" )
    {
      invertedIndex->updateDocumentsBase(converterJSON->getDocumentPaths());
    }
    else
    if ( command == "2" )
    {
      //получаем, печатаем список содержимого валидных файлов:
      auto vDocs = invertedIndex->getTextsDocs();
      auto itDocs = vDocs.begin();
      auto vPaths = invertedIndex->getValidDocsPaths();
      int i = 0;

      for ( auto itPath = vPaths.begin(); itPath != vPaths.end() && !vPaths.empty(); itPath++, itDocs++, i++)
        std::cout << "\tФайл " << i << ". \"" << *itPath << "\":\n\t" << *itDocs << "\n\n";
    }
    else
    if ( command == "3" )
    {
      //распечатать частотный словарь:
      int i = 0;
      for ( const auto elem : invertedIndex->getFreq_dictionary() )
      {
        std::cout << "\t" << elem.first;
          for ( int j = 0; j < elem.second.size(); j++ )
            std::cout << " {" << elem.second[j].doc_id << ", " 
                              << elem.second[j].count << "}";
        std::cout << std::endl;
        i++;
      }
    }
    else
    if ( command == "4" )
    {
      //вывод количества вхождений слова в базе документов {индекс_файла, количество_слова_в_файле}:
      std::string query;
      std::cin.ignore(); //нужно для getline() - пропускаем один символ '\n' после ввода command

      while ( true )
      {
        std::cout << "\nВведите запрос (запрос - это одно или несколько слов) (-1 - назад): ";
        
        std::getline(std::cin, query);

        if ( query == "-1" )  break;

        std::vector<std::string> vecQueries;
        vecQueries.push_back(query);

        showRelevanceRezult( searchServer->search(vecQueries, converterJSON->getResponsesLimit()) );
      }
    }
    else
    if ( command == "5" )
    {
      std::cout << std::endl;
      for ( auto req : converterJSON->getRequests() )
        std::cout << "\t" <<req << std::endl;
    }
    else
    if ( command == "6" )
    {
      auto searchResult = searchServer->search( converterJSON->getRequests(), converterJSON->getResponsesLimit() );
      showRelevanceRezult(searchResult); //печатаем в консоль

      //т.к. релевантности запросам в разных классах хранятся в разных типах контейнеров - сделаем преобразование из ответа 
      //vector<vector<RelativeIndex>> SearchServer::search() в результат, нужный для последующей сериализации в виде 
      //vector<vector<pair<int, float>>> ConverterJSON::putAnswers():
      std::vector<std::vector<std::pair<int, float>>> relevancesQuerieS; //релевантности всем запросам
      std::vector<std::pair<int, float>> relevanceQuery; //релевантность одному запросу

      for ( auto vecOut : searchResult )
      {
        relevanceQuery.clear();
        relevanceQuery.shrink_to_fit();

        for ( auto structOut : vecOut )
          relevanceQuery.push_back( {static_cast<std::size_t>(structOut.doc_id), structOut.rank} );

        relevancesQuerieS.push_back(relevanceQuery);
      }

      converterJSON->putAnswers(relevancesQuerieS); //сериализация ответа поисковика
    }

    if ( command != "4" )
    {
      std::cout << "\nнажмите Enter..."; 
      std::cin.get(); //обрабатываем один Enter после ввода command
      std::cin.get(); //задаём ожидание ввода
    }
  }


  delete searchServer;
  delete invertedIndex;
  delete converterJSON;
}