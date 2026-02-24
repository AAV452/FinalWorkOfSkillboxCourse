#pragma  once //прагма - команда прекомпилятора - исключает повторение и пересечение вызовов
              //данного .h-файла в других срр-файлах и разных сценариях проекта

#include <invertedIndex.h>

//функция создаёт список уникальных слов из входной строки:
std::vector<std::string> listUniqueWords(std::string str); //функция будет исп-ся в потоке в лямбде - поэтому параметр не по ссылке


struct RelativeIndex
{
  size_t doc_id;
  float rank;

  bool operator == ( const RelativeIndex &other ) const
  {
    return ( doc_id == other.doc_id && rank == other.rank);
  }
};


class SearchServer
{
private:

  InvertedIndex _index;

public:
  
  //@param idx в конструктор класса передаётся ссылка на класс InvertedIndex, чтобы SearchServer мог узнать частоту слов,
  //встречаемых в запросе
  SearchServer( InvertedIndex &idx ) : _index (idx) {};

  ~SearchServer() { /*std::cout << "Класс SearchServer отработал!!!";*/ };

  //Метод обработки поисковых запросов:
  //@param queries_input - поисковые запросы, взятые из файла requests.json
  //@return возвращает отсортированный список релевантных ответов для заданных запросов
  std::vector<std::vector<RelativeIndex>> search (const std::vector<std::string> &queries_input, int max_responses = 5);
};