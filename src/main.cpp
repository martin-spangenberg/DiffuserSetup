#include <string>
#include <unistd.h>
#include "ToolChain.h"
#include "DummyTool.h"

int main(int argc, char* argv[])
{

  int nargs = 0;
  char* args[100];

  std::string conffile = "configfiles/Main/ToolChainConfig";
  for (int i=0; i<argc; ++i)
  {
    if(strncmp(argv[i], "-xxxxxxxxxxxxxx", 1) == 0 && i != argc-1)
    {
      if(strcmp(argv[i], "--chainconf") == 0)
        conffile = argv[i+1];
      else
      {
        std::cout << "Command line flag \"" << argv[i] << "\" not recognised!" << std::endl;
        return 1;
      }
      ++i;
    }
    else
    {
      args[nargs] = argv[i];
      nargs += 1;
    }
  }

  if(nargs < 2)
  {
    std::cout << "Usage:" << std::endl;
    std::cout << "./main <configfile> [ <outputfile> --chainconf <ToolChain configfile> ]" << std::endl;
    return 1;
  }

  ToolChain tools(conffile, nargs, args);

  //DummyTool dummytool;    

  //tools.Add("DummyTool",&dummytool,"configfiles/DummyToolConfig");

  //int portnum=24000;
  //  tools.Remote(portnum);
  //tools.Interactive();
  
  //  tools.Initialise();
  // tools.Execute();
  //tools.Finalise();
  
  
  return 0;
  
}
