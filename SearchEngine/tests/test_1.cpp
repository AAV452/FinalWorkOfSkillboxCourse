#include <gtest/gtest.h>

#include <converterJSON.h>
#include <invertedIndex.h>
#include <searchServer.h>

#include <filesystem> //подключаем библиотеку для создания папки
#include <fstream>

//using namespace std;

//Т.к. функция InvertedIndex::updateDocumentsBase(<список_путей>) принимает на вход список путей к файлам (по которым
//осуществляется поиск), а не список содержимого этих файлов, то для корректного и наглядного тестирования проще 
//будет внутрь тестов передавать не сами пути, а непосредственно содержимое файлов в виде коллекций. Поэтому напишем 
//функцию, которая по входной коллекции содержимого файлов, создаваемых в тестах, будет создавать временные и папку,
//и эти файлы и возвращать список путей к ним. А уже этот список будет передаваться на вход
//InvertedIndex::updateDocumentsBase(<список_путей>) в тестах:

//функция принимает вектор содержимого файлов, разбивает его на файловые-тексты и создаёт по каждому свой
//текстовый файл в папке docsFolder:
//(используется в тестах для обработки массива текстов)
std::vector<std::string> createDocsFiles( const std::vector<std::string> &docs, std::string docsFolder )
{
  if ( !std::filesystem::create_directory(docsFolder) )
  {
    std::cout << "Папка с документами \"" << docsFolder << "\" не создана\n";
    return {};
  }

  std::string path = std::filesystem::current_path().string(); //узнаём текущий путь
  //для корректности пути заменяем все \ на \\:
  std::string oldStr = "\\",
              newStr = "\\\\";
  size_t pos = 0;
  //std::string::npos - константа, обозначающая "позиция не найдена".
  //find(<искомое>, <позиция_начала>) возвратит индекс (тип size_t) первого символа найденной подстроки или npos.
  while ( (pos = path.find(oldStr, pos)) != std::string::npos )
  {
    path.replace(pos, oldStr.length(), newStr); //удалит с pos символы количеством oldStr.length(), и вставит тудаn newStr
    pos += newStr.length();
  }
  
  std::vector<std::string> result;

  //создадим временные файлы, а их пути используем в тестах для запуска метода InvertedIndex::updateDocumentsBase(<список_путей>):
  int i = 0;
  for ( auto text : docs )
  {
    std::string str = path + "\\\\" + docsFolder + "\\\\text" + std::to_string(i) + ".txt";

    std::ofstream file(str);
    file << text;
    file.close();

    result.push_back(str);

    i++;
  }

  return result;
}


void testInvertedIndexFunctionality( const std::vector<std::string> &docs,
                                     const std::vector<std::string> &requests,
                                     const std::vector<std::vector<Entry>> &expected )
{
  std::vector<std::vector<Entry>> result;

  InvertedIndex idx;
  idx.updateDocumentsBase( createDocsFiles(docs, "docsFolder") ); //заполняем частотный словарь
  std::filesystem::remove_all( "docsFolder" ); //удаляем временную папку

  for( auto &request : requests )
  {
    std::vector<Entry> word_count = idx.getWordCount(request);
    result.push_back(word_count);
  }

  //один из макросов в юнит-тестах для проверки равенства двух значений. Если не равны - тест завершается с фатальной
  //ошибкой (fatal failule), и последующие команды в этом тесте не выполняются, сообщая о не совпадении.
  ASSERT_EQ(result, expected);
}



//TEST(<имя_группы_тестов>, <уникальное_имя_сценария_проверки>) - макрос для определения модульного теста (unit test). Он
//создаёт функцию, проверяющую корректность отдельной части кода, не требуя функции main(). Принимает название набора
//тестов и уникальное имя теста, содержащего проверки (EXPECT_EQ, ASSERT_TRUE, ASSERT_EQ и др.)
TEST(testCaseInvertedIndex, testBasic)
{
  const std::vector<std::string> docs = { "london is the capital of great britain",
                                          "bigben is the nickname for the Great bell of the striking clock"
                                        };
  const std::vector<std::string> requests = { "london", "the" };
  const std::vector <std::vector<Entry>> expected = { { {0,1} },
                                                      { {0,1}, {1,3} }
                                                    };

  testInvertedIndexFunctionality(docs, requests, expected);
}

TEST(testCaseInvertedIndex, testBasic2)
{
  const std::vector<std::string> docs = { "milk milk milk milk water water water",
                                          "milk water water",
                                          "milk milk milk milk milk water water water water water",
                                          "americano cappuccino"
                                        };                                     
  const std::vector<std::string> requests = { "milk", "water", "cappuccino" };
  const std::vector<std::vector<Entry>> expected = { { {0,4},{1,1},{2,5} },
                                                     { {0,3},{1,2},{2,5} },
                                                     { {3,1} }
                                                   };
  testInvertedIndexFunctionality(docs, requests, expected);
}

TEST(testCaseInvertedIndex, testInvertedIndexMissingWord)
{
  const std::vector<std::string> docs = { "abcdefghijkl", "statement" };
  const std::vector<std::string> requests = { "m", "statement" };
  const std::vector<std::vector<Entry>> expected = { {},
                                                     { {1,1} }
                                                   };
  testInvertedIndexFunctionality(docs, requests, expected);
}




TEST(TestCaseSearchServer, TestSimple)
{ 
  const std::vector<std::string> docs = { "milk milk milk milk water water water",
                                          "milk water water",
                                          "milk milk milk milk milk water water water water water",
                                          "americano cappuccino"
                                        };
  const std::vector<std::string> request = { "milk water", "sugar" };
  const std::vector<std::vector<RelativeIndex>> expected = { { {2, 1},
                                                               {0, 0.7},
                                                               {1, 0.3} },
                                                             {}
                                                           };

  InvertedIndex idx;
  idx.updateDocumentsBase( createDocsFiles(docs, "docsFolder") ); //заполняем частотный словарь
  std::filesystem::remove_all( "docsFolder" ); //удаляем временную папку

  SearchServer srv(idx);
  std::vector<std::vector<RelativeIndex>> result = srv.search(request);
  ASSERT_EQ(result, expected);
}

TEST(TestCaseSearchServer, TestTop5)
{ 
  const std::vector<std::string> docs = {
                                          "london is the capital of great britain",
                                          "paris is the capital of france",
                                          "berlin is the capital of germany",
                                          "rome is the capital of italy",
                                          "madrid is the capital of spain",
                                          "lisboa is the capital of portugal",
                                          "bern is the capital of switzerland",
                                          "moscow is the capital of russia",
                                          "kiev is the capital of ukraine",
                                          "minsk is the capital of belarus",
                                          "astana is the capital of kazakhstan",
                                          "beijing is the capital of china",
                                          "tokyo is the capital of japan",
                                          "bangkok is the capital of thailand",
                                          "welcome to moscow the capital of russia the third rome",
                                          "amsterdam is the capital of netherlands",
                                          "helsinki is the capital of finland",
                                          "oslo is the capital of norway",
                                          "stockholm is the capital ofsweden",
                                          "riga is the capital of latvia",
                                          "tallinn is the capital of estonia",
                                          "warsaw is the capital of poland"
                                        };
  const std::vector<std::string> request ={"moscow is the capital of russia"};
  const std::vector<std::vector<RelativeIndex>> expected ={
                                                            {//в программе после сортировки ответа по уменьшению релевантности
                                                              {7,1}, //0-ой поменялся с 7-ым
                                                              {14,1},//1-ый поменялся с 14-м
                                                              {2,0.666666687},
                                                              {3,0.666666687},
                                                              {4,0.666666687}
                                                            }
                                                          };
  InvertedIndex idx;
  idx.updateDocumentsBase( createDocsFiles(docs, "docsFolder") ); //заполняем частотный словарь
  std::filesystem::remove_all( "docsFolder" ); //удаляем временную папку

  SearchServer srv(idx);
  std::vector<std::vector<RelativeIndex>> result = srv.search(request); //5 ответов по умолчанию
  ASSERT_EQ(result,expected);
}

/*
TEST(sample_test_case, sample_test)
{
  EXPECT_EQ(1,1);
}

TEST(sample_test_case, sample_test2)
{
  EXPECT_EQ(1,1);
}
*/
/*
TEST(ExamleTest, Test1)
{
  EXPECT_EQ(5 + 5, add(5, 5));
}
*/


int main(int argc, char * argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}