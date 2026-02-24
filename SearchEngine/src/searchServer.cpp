#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>

#include "searchServer.h"
#include "converterJSON.h"
//#include "invertedIndex.h"


//функция создаёт список уникальных слов из входной строки:
std::vector<std::string> listUniqueWords(std::string str) //функция будет исп-ся в потоке в лямбде - поэтому параметр не по ссылке
{
  std::vector<std::string> vecResult = {}; //список уникальных слов запроса
  std::string word = "";
  bool flag = false; //слово не сформировано

  //проходим по тексту запроса и собираем список уникальных слов:
  for ( int j = 0; j < str.size(); j++ )
  {
    //если символ - цифра, буква..., то начать формировать слово:
    if ( ( str[j] >= '0' && str[j] <= '9' ) ||
         ( str[j] >= 'a' && str[j] <= 'z' ) ||
         ( str[j] >= 'A' && str[j] <= 'Z' ) )
    //начинаем формировать слово из букв/цифр:
    {
      word += str[j];
      flag = false;
    }
    else
    if ( word != "" && !flag) //иначе, если символ - не цифра/буква, а слово начато, то слово сформировано
      flag = true;

    if ( word != "" && j == str.size() - 1 ) //если последний символ текста - цифра/буква, то слово сформировано
      flag = true;

    if ( flag )
    { 
      //проверяем сформированное слово на уникальность среди предыдущих слов строки через лямбда-функцию в условии:
      //()->bool можно не писать!!!!!!! (это синтаксис для себя)
      if ( [vecResult, word] ()->bool { for ( auto uWord : vecResult )
                                          if ( uWord == word )
                                            return false;

                                        return true; //если for отработал                               
                                      }() // () - это вызов лямбды
      )
        vecResult.push_back(word); //если слово уникальное - заносим его в список

      word = ""; //готовимся к следующему слову
      flag = false;
    }
  }

  return vecResult;
}

//на входе вектор запросов ConverterJSON.requestsVec, созданный при старте поискового движка:
std::vector<std::vector<RelativeIndex>> SearchServer::search (const std::vector<std::string> &queries_input, int max_responses)
{
  std::vector<std::vector<RelativeIndex>> resultRelevantAnswers;

  auto freq_dict = _index.getFreq_dictionary(); //частотный словарь

  std::vector<std::thread> threads;
  std::mutex mtxAdd;

  std::vector<RelativeIndex> vecRelat;

  //обсчитываем каждый запрос (запрос - это строка из одного или нескольких слов):
  for ( auto query : queries_input ) 
  {
    vecRelat.clear();
    
//    threads.emplace_back([&, query]()
//    {
      //словарь автоматически упорядочивает записи по ключу при добавлении в словарь нового элемента + быстрый бинарный поиск
      //1. поэтому создадим временный словарь (документ -> его абсол. релевантность запросу) для последующего расчёта итоговой относит.
      //релевантности каждого документа этому запросу (заполнив этот словарь, увидим документ с максимальной абслолютной релевантностью
      //запросу, и по ней вычислим итоговые релевантности остальных документов):
      std::map<int, int> docAbsoluteRank; //key - номер документа (по нему бкдет автоматическая сортировка), 
                                          //value - абсолютная релевантность этого документа запросу (т.е. сумма
                                          //count-ов всех уникальных слов из запроса в этом документе).
      int maxAbsoluteRank = 0; //max абслолютная релевантность запросу (макс. value из docAbsoluteRank)

      //2. разбиваем запрос на уникальные слова и создаём из них список
      auto uniqWords = listUniqueWords(query);
    
      for ( auto const word : uniqWords )
      {
        //каждое уникальное слово запроса ищем в частотном словаре, чтобы получить по нему либо список документов,
        //в которых это слово встречается, либо пустой список, если слова нет ни в одном документе:
        std::vector<Entry> wordVec = {};

        auto const it = freq_dict.find(word);

        if ( it != freq_dict.end() ) //если слово найдено в частотном словаре - берём вектор его вхождений в документы
          wordVec = it->second;
      
        //3. по вектору вхождений слова заполняем наш временный словарь релевантности (в каждом документе нужно посчитать сумму
        //вхождений всех слов из запроса):
        //пр.: есть док №0: "а б а в" и есть запрос найти: "а б". Смотрим, что в доке №0 слов "а" есть 2 и слов "б" есть 1, итого, 
        //2+1=3 слова из запроса найдено в доке №0, т.е {0, 3}
        for ( auto const i : wordVec )
        {
          auto findElem = docAbsoluteRank.find(i.doc_id); //номер документа ищем в нашем временном словаре релевантности
          if ( findElem == docAbsoluteRank.end() )
          {
            docAbsoluteRank.insert( {i.doc_id, i.count} );
            if ( i.count > maxAbsoluteRank ) 
              maxAbsoluteRank = i.count;
          }
          else
          {
            findElem->second += i.count; //если в документе уже были слова из запроса, добавляем и наше, т.о. считаем
                                         //абслолютную релевантность всех докумементов запросу и заодно максимальную
            if ( findElem->second > maxAbsoluteRank ) 
              maxAbsoluteRank = findElem->second;
          }
        }                                                 
      } //так посчитана абслолютная релевантность каждого документа этому запросу, а из них взята максимальная релевантность.

      //заносим результаты в ответ, расчитывая относительную релевантность:
      //std::vector<RelativeIndex> vecRelat;
      RelativeIndex ri = {};
      for ( auto ourMap : docAbsoluteRank )
      {
        ri.doc_id = static_cast<std::size_t>(ourMap.first);
        ri.rank = (float)((float)ourMap.second/(float)maxAbsoluteRank);
        vecRelat.emplace_back(ri);
      }

      //сортируем ответ пузырьком по убыванию относительной релевантности:
      for ( int i = 0; i < vecRelat.size(); i++ )
      {
        float max = vecRelat[i].rank;

        for ( int j = i + 1; j < vecRelat.size(); j++ )
          if ( vecRelat[j].rank > max )
          {
            max = vecRelat[j].rank;
            RelativeIndex tempMax;
            tempMax = vecRelat[j];
            vecRelat[j] = vecRelat[i];
            vecRelat[i] = tempMax;
          }
      }

      //удаляем лишние ответы согласно лимиту из ConverterJSON.config.max_responses:
      if ( vecRelat.size() > max_responses )
      {
        vecRelat.resize(max_responses);
        vecRelat.shrink_to_fit(); //освобождение памяти
      }

      //////////////////////////////////////////////////
      resultRelevantAnswers.push_back(vecRelat); //если без потоков

//      return vecRelat;
//    });
  }

  /*
 Я отказался от расчёта каждого запроса в отдельном потоке, потому что не смог сделать завершение потоков в 
 правильном порядке!!!!!  Они заполняли вектор ответа вразнобой, хотя ниже я использовал цикл t.join()!!!!!!!!!!

  //запускаем цикл ожидания завершения всех потоков в порядке их запуска в векторе - синхронизацию:
  for ( auto &t : threads )
    //if ( t.joinable() )
    {
    //сначала записываем ответ, потом завершаем поток:
      mtxAdd.lock();
        resultRelevantAnswers.push_back(vecRelat);
      mtxAdd.unlock();

      t.join();
    }
  */

  return resultRelevantAnswers;
}