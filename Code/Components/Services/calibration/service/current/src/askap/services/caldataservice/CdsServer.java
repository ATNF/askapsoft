package askap.services.caldataservice;

import askap.util.ParameterSet;
import askap.util.ServiceApplication;
import askap.util.ServiceManager;

/**
 * 
 * 
 * Copyright 2017, CSIRO Australia All rights reserved.
 * 
 */
public class CdsServer extends ServiceApplication
{

  /**
   * Starts the server
   * @param args program arguments as required by ICE
   */
  public static void main( String[] args )
  {
    CdsServer app = new CdsServer() ;
    int status = app.servicemain( args ) ;
    System.exit( status ) ;
  }

  @Override
  public int run( String[] args )
  {

    CdsServerI server ;
    ParameterSet properties = config() ;
    try
    {
      server = new CdsServerI( properties );
    } catch ( Exception e )
    {
      System.err.println( e.getMessage() ) ;
      return 1 ;
    }
    String adapterName = server.getAdapterName() ;
    String serviceName = server.getServerName() ;
    ServiceManager manager = new ServiceManager() ;
    manager.start( communicator(), server, serviceName, adapterName ) ;
    manager.waitForShutdown() ;
    manager.stop() ;
    
    return 0;
  }

}
