package askap.services.caldataservice;

import org.apache.log4j.Logger;

import Ice.Current;
import askap.interfaces.caldataservice.AlreadyExists;
import askap.interfaces.caldataservice.UnknownSolutionIdException;
import askap.interfaces.calparams.TimeTaggedBandpassSolution;
import askap.interfaces.calparams.TimeTaggedGainSolution;
import askap.interfaces.calparams.TimeTaggedLeakageSolution;
import askap.util.ParameterSet;


/**
 * Calibration Data Service server functions implementation
 * 
 * Copyright 2017, CSIRO Australia
 * All rights reserved.
 * 
 */
public class CdsServerI extends askap.interfaces.caldataservice._ICalibrationDataServiceDisp
{  
  /** Logger */
  protected static final Logger LOG = Logger.getLogger(CdsServerI.class.getName());

  
  /** Compulsory for Serializable version number */
  private static final long serialVersionUID = 1L;
  
  /** MySQL based storage */
  private MySQLStorage storage ;
  
  /** ICE adapter name */
  private String adapterName ;
  
  /** ICE server name */
  private String serverName ;
  
  /** ICE server address */
  private String serverAddress ;


  /**
   * Constructor: creates server object based on values taken from ICE properties file.
   * 
   * @param properties - ICE properties object
   * @throws Exception 
   * 
   */
  public CdsServerI( ParameterSet properties ) throws Exception
  {
  
    adapterName = properties.getString( "adapter.name" ) ;
    serverName = properties.getString( "server.name" ) ;
    serverAddress = properties.getString( "server.address" ) ;
    String dbUrl = properties.getString( "db.url" ) ;
    String dbDriver = properties.getString( "db.driver" ) ;
    String dbUser = properties.getString( "db.user" ) ;
    String dbPassword = properties.getString( "db.password" ) ;
    String storageDir = properties.getString( "storage.dir" ) ;
        
    storage = new MySQLStorage( dbUrl, dbDriver, dbUser, dbPassword, storageDir ) ;
  }





  public String getAdapterName()
  {
    return adapterName;
  }

  public void setAdapterName( String adapterName )
  {
    this.adapterName = adapterName;
  }

  public String getServerName()
  {
    return serverName;
  }

  public void setServerName( String serverName )
  {
    this.serverName = serverName;
  }

  public String getServerAddress()
  {
    return serverAddress;
  }

  public void setServerAddress( String serverAddress )
  {
    this.serverAddress = serverAddress;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#newSolutionID(Ice.Current)
   */
  @Override
  public long newSolutionID( Current __current )
  {
    return storage.newSolutionID() ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#addGainsSolution(long, askap.interfaces.calparams.TimeTaggedGainSolution, Ice.Current)
   */
  @Override
  public void addGainsSolution( long id, TimeTaggedGainSolution solution, Current __current ) throws AlreadyExists
  {
    storage.addGainsSolution( id, solution ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#addBandpassSolution(long, askap.interfaces.calparams.TimeTaggedBandpassSolution, Ice.Current)
   */
  @Override
  public void addBandpassSolution( long id, TimeTaggedBandpassSolution solution, Current __current )
      throws AlreadyExists
  {
    storage.addBandpassSolution( id, solution ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#addLeakageSolution(long, askap.interfaces.calparams.TimeTaggedLeakageSolution, Ice.Current)
   */
  @Override
  public void addLeakageSolution( long id, TimeTaggedLeakageSolution solution, Current __current ) throws AlreadyExists
  {
    storage.addLeakageSolution( id, solution ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#adjustGains(long, askap.interfaces.calparams.TimeTaggedGainSolution, Ice.Current)
   */
  @Override
  public void adjustGains( long id, TimeTaggedGainSolution solution, Current __current ) throws AlreadyExists
  {
    storage.adjustGains( id, solution ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#adjustBandpass(long, askap.interfaces.calparams.TimeTaggedBandpassSolution, Ice.Current)
   */
  @Override
  public void adjustBandpass( long id, TimeTaggedBandpassSolution solution, Current __current ) throws AlreadyExists
  {
    storage.adjustBandpass( id, solution ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#adjustLeakages(long, askap.interfaces.calparams.TimeTaggedLeakageSolution, Ice.Current)
   */
  @Override
  public void adjustLeakages( long id, TimeTaggedLeakageSolution solution, Current __current ) throws AlreadyExists
  {
    storage.adjustLeakages( id, solution ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#hasGainSolution(long, Ice.Current)
   */
  @Override
  public boolean hasGainSolution( long id, Current __current )
  {
    return storage.hasGainSolution( id ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#hasLeakageSolution(long, Ice.Current)
   */
  @Override
  public boolean hasLeakageSolution( long id, Current __current )
  {
    return storage.hasLeakageSolution( id ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#hasBandpassSolution(long, Ice.Current)
   */
  @Override
  public boolean hasBandpassSolution( long id, Current __current )
  {
    return storage.hasBandpassSolution( id ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#getLatestSolutionID(Ice.Current)
   */
  @Override
  public long getLatestSolutionID( Current __current ) throws UnknownSolutionIdException
  {
    return storage.getLatestSolutionID() ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#getUpperBoundID(double, Ice.Current)
   */
  @Override
  public long getUpperBoundID( double timestamp, Current __current ) throws UnknownSolutionIdException
  {
    return storage.getUpperBoundID( timestamp ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#getLowerBoundID(double, Ice.Current)
   */
  @Override
  public long getLowerBoundID( double timestamp, Current __current ) throws UnknownSolutionIdException
  {
    return storage.getLowerBoundID( timestamp ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#getGainSolution(long, Ice.Current)
   */
  @Override
  public TimeTaggedGainSolution getGainSolution( long id, Current __current ) throws UnknownSolutionIdException
  {
    return storage.getGainSolution( id );
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#getLeakageSolution(long, Ice.Current)
   */
  @Override
  public TimeTaggedLeakageSolution getLeakageSolution( long id, Current __current ) throws UnknownSolutionIdException
  {
    return storage.getLeakageSolution( id ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.caldataservice._ICalibrationDataServiceOperations#getBandpassSolution(long, Ice.Current)
   */
  @Override
  public TimeTaggedBandpassSolution getBandpassSolution( long id, Current __current ) throws UnknownSolutionIdException
  {
    return storage.getBandpassSolution( id ) ;
  }


  /* (non-Javadoc)
   * @see askap.interfaces.services._IServiceOperations#getServiceVersion(Ice.Current)
   */
  public String getServiceVersion( Current __current )
  {
    try {
      Package p = this.getClass().getPackage();
      return p.getImplementationVersion();
  } catch (Exception e) {
      LOG.error("waitObs() - Unexpected exception: " + e.getMessage());
      LOG.error(e.getStackTrace());
      throw new Ice.UnknownException(e.getMessage());
  }

  }


}
