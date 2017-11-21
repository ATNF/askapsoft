package askap.services.caldataservice;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInput;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

import askap.interfaces.caldataservice.ICalibrationDataServicePrx;
import askap.interfaces.caldataservice.ICalibrationDataServicePrxHelper;
import askap.interfaces.caldataservice.UnknownSolutionIdException;
import askap.interfaces.calparams.TimeTaggedBandpassSolution;
import askap.interfaces.calparams.TimeTaggedGainSolution;
import askap.interfaces.calparams.TimeTaggedLeakageSolution;


/**
 * Implements ACDS ICE client
 * 
 * Copyright 2017, CSIRO Australia
 * All rights reserved.
 * 
 */
public class CdsClient extends Ice.Application
{
  /** Data object used in operations */
  Serializable object ;
  
  /** Output stream to save result to */
  ObjectOutputStream out ; 
  
  /** Object id, if required by operation */
  long id ;
   
  /** ICE proxy */
  ICalibrationDataServicePrx proxy ;
  
  /** Operation result code */
  long result ;
  
  public static void main( String[] args )
  {
    if ( args.length == 0 )
    {
      System.out.println( "Arguments: <path to properties file> <operation> [<operation parameters>]" );
      System.exit( -1 ) ;
    }

    
    long result = -1 ;
    CdsClient client = null ;
    try( InputStream input = new FileInputStream( args[ 0 ] ) ; )
    {
      client = new CdsClient() ;
      client.main( "CdsClient", args, args[ 0 ] ) ;
    }
    catch( Throwable e )
    {
      System.out.print( e.getMessage() ) ;
    }

    if ( client != null ) result = client.result ;
    System.out.print( result ) ;
    System.exit( (int)result ) ;
  }
  
  
  /**
   * Request processor. Always expects name of the operation to perform in the second parameter
   * and ICE properties file name in the first parameter. Other parameters are interpreted
   * depending on the operation.
   * 
   * @param args - original program arguments
   * @param communicator 
   * @return 1 if failed, else 0
   * Depending on operation, sets result field to result code
   */
  public int run( String[] args )
  {
    
    String action = args[ 1 ] ;

    
    Ice.Properties properties = communicator().getProperties() ;
    String serverName = properties.getProperty( "server.name" ) ;
    String serverType = properties.getProperty( "server.type" ) ;

    try
      {
        proxy = ICalibrationDataServicePrxHelper.checkedCast(communicator().stringToProxy( serverName ) ) ;
      }
    catch( Ice.NotRegisteredException ex ) {}
    
    if( proxy == null )
      {
        System.err.println( "couldn't find a '" + serverType + "' object" ) ;
        result = -1 ;
        return 1 ;
      }

    try
    {
      switch( action )
       {
        case "newSolutionID" : result = newSolutionID() ; break ;
        case "addGainsSolution" : result = addGainsSolution( args ) ; break ;
        case "addBandpassSolution" : result = addBandpassSolution( args ) ; break ;
        case "addLeakageSolution" : result = addLeakageSolution( args ) ; break ;
        case "adjustGains" : result = adjustGains( args ) ; break ;
        case "adjustBandpass" : result = adjustBandpass( args ) ; break ;
        case "adjustLeakages" : result = adjustLeakages( args ) ; break ;
        case "hasGainSolution" : result = hasGainSolution( args ) ; break ;
        case "hasLeakageSolution" : result = hasLeakageSolution( args ) ; break ;
        case "hasBandpassSolution" : result = hasBandpassSolution( args ) ; break ;
        case "getLatestSolutionID" : result = getLatestSolutionID() ; break ;
        case "getUpperBoundID" : result = getUpperBoundID( args ) ; break ;
        case "getLowerBoundID" : result = getLowerBoundID( args ) ; break ;
        case "getGainSolution" : result = getGainSolution( args ) ; break ;
        case "getLeakageSolution" : result = getLeakageSolution( args ) ; break ;
        case "getBandpassSolution" : result = getBandpassSolution( args ) ; break ;
        case "test" : result = test() ; break ;
        default: throw new Exception( "Invalid action: " + action ) ;
       }
    }
    catch( Throwable e )
    {
      System.err.println( "Failed: " + e ) ;
      result = -1 ;
      return 1 ;
    }
   
   return 0 ;
  }

  
  
  /**
   * Does a basic server connectivity/health test. Should not be overused because it does save a few 
   * small test solutions on the server.
   * 
   * @return 0 if passed
   * @throws Exception 
   * @throws Exception or -1 if failed
   */
  private long test() throws Exception 
  {
    // Create test objects 
    TimeTaggedGainSolution gSol0 = Utils.makeGainSolution( 21 ), gSol1 = null ;
    TimeTaggedLeakageSolution lSol0 = Utils.makeLeakageSolution( 22 ), lSol1 = null ;
    TimeTaggedBandpassSolution bSol0 = Utils.makeBandpassSolution( 23 ), bSol1 = null ;
    // Test ID generation
    
    long id1 = newSolutionID() ;
    long id2 = newSolutionID() ;
    if ( id2 <= id1 )
      throw new RuntimeException( "New solution id generation failed: returned sequential ids are: " + id1 + " and " + id2 ) ;
    if ( id2 - id1 > 1 )
      throw new RuntimeException( "WARNING: returned sequential new solution ids are: " + id1 + " and " + id2 ) ;
    long id3 = getLatestSolutionID() ;
    if ( id3 < id2 )
      throw new RuntimeException( "Getting latest solution failed: returned ids are: " + id2 + " and " + id3 ) ;
    if ( id3 - id2 > 1 )
      throw new RuntimeException( "WARNING: returned latest solution id is: " + id3 + " after latest new " + id2 ) ;
    
    proxy.addBandpassSolution( id1, bSol0 ) ;
    proxy.addLeakageSolution( id1, lSol0 ) ;
    proxy.addGainsSolution( id1, gSol0 ) ;
    
    try
    {
      proxy.addBandpassSolution( id1, bSol0 ) ;
      throw new RuntimeException( "Did not detect repeated adding of a bandpass solution." ) ;
    } catch( Exception e ) {}
    
    try
    {
      proxy.addGainsSolution( id1, gSol0 ) ;
      throw new RuntimeException( "Did not detect repeated adding of a gains solution." ) ;
    } catch( Exception e ) {}
    
    try
    {
      proxy.addLeakageSolution( id1, lSol0 ) ;
      throw new RuntimeException( "Did not detect repeated adding of a leakage solution." ) ;
    } catch( Exception e ) {}
    
    bSol1 = proxy.getBandpassSolution( id1 ) ;
    if ( !bSol0.equals( bSol1 ) )
      throw new RuntimeException( "Saved and read bandpass solutions are not identical." ) ;
    
    gSol1 = proxy.getGainSolution( id1 ) ;
    if ( !gSol0.equals( gSol1 ) )
      throw new RuntimeException( "Saved and read gains solutions are not identical." ) ;
    
    lSol1 = proxy.getLeakageSolution( id1 ) ;
    if ( !lSol0.equals( lSol1 ) )
      throw new RuntimeException( "Saved and read leakage solutions are not identical." ) ;

    try
    {
      bSol1 = proxy.getBandpassSolution( id2 ) ;
      throw new RuntimeException( "Could read a missing bandpass solution." ) ;
    } catch( Exception e ) {}

    try
    {
      gSol1 = proxy.getGainSolution( id2 ) ;
      throw new RuntimeException( "Could read a missing gains solution." ) ;
    } catch( Exception e ) {}

    try
    {
      lSol1 = proxy.getLeakageSolution( id2 ) ;
      throw new RuntimeException( "Could read a missing leakage solution." ) ;
    } catch( Exception e ) {}

    return 0;
  }


  /**
   * Adds TimeTaggedLeakageSolution. Expects solution ID in args[ 2 ] and 
   * name of file to read the solution from in args[ 3 ]. 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if failed 
   */
  private int addLeakageSolution( String[] args ) throws Exception
  {
    getLongAndObject( args ) ;
    proxy.addLeakageSolution( id, (TimeTaggedLeakageSolution)object ) ;
    return 0;
  }


  /**
   * Adds TimeTaggedGainsSolution. Expects solution ID in args[ 2 ] and 
   * name of file to read the solution from in args[ 3 ]. 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if failed 
   */
  private int addGainsSolution( String[] args ) throws Exception
  {
    getLongAndObject( args ) ;
    proxy.addGainsSolution( id, (TimeTaggedGainSolution)object ) ;
    return 0;
  }


  /**
   * Adds TimeTaggedBandpassSolution. Expects solution ID in args[ 2 ] and 
   * name of file to read the solution from in args[ 3 ]. 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if failed 
   */
  private int addBandpassSolution( String[] args ) throws Exception
  {
    getLongAndObject( args ) ;
    proxy.addBandpassSolution( id, (TimeTaggedBandpassSolution)object ) ;
    return 0;
  }


  /**
   * Requests issuing of a new solution ID.
   * @return new solution ID.
   */
  private long newSolutionID()
  {
    return proxy.newSolutionID() ;
  }


  /**
   * Adjusts TimeTaggedGainsSolution. Expects solution ID in args[ 2 ] and 
   * name of file to read the solution from in args[ 3 ]. 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if failed 
   */
  private int adjustGains( String[] args ) throws Exception
  {
    getLongAndObject( args ) ;
    proxy.addGainsSolution( id, (TimeTaggedGainSolution)object ) ;
    return 0;
  }
  

  /**
   * Adjusts TimeTaggedBandpassSolution. Expects solution ID in args[ 2 ] and 
   * name of file to read the solution from in args[ 3 ]. 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if failed 
   */
  private int adjustBandpass( String[] args ) throws Exception
  {
    getLongAndObject( args ) ;
    proxy.adjustBandpass( id, (TimeTaggedBandpassSolution)object ) ;
    return 0;
  }


  /**
   * Adjusts TimeTaggedLeakageSolution. Expects solution ID in args[ 2 ] and 
   * name of file to read the solution from in args[ 3 ]. 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if failed 
   */
  private int adjustLeakages( String[] args ) throws Exception
  {
    getLongAndObject( args ) ;
    proxy.adjustLeakages( id, (TimeTaggedLeakageSolution)object ) ;
    return 0;
  }


  /**
   * Checks if solution with this id has a gains component.
   * @param args - original program args
   * @return 0 if no, 1 if yes
   */
  private int hasGainSolution( String[] args )
  {
    id = getLongParam( args ) ;
    return proxy.hasGainSolution( id ) ? 1 : 0 ;
  }


  /**
   * Checks if solution with this id has a leakage component.
   * @param args - original program args
   * @return 0 if no, 1 if yes
   */
  private int hasLeakageSolution( String[] args )
  {
    id = getLongParam( args ) ;
    return proxy.hasLeakageSolution( id ) ? 1 : 0 ;
  }


  /**
   * Checks if solution with this id has a bandpass component.
   * @param args - original program args
   * @return 0 if no, 1 if yes
   */
  private int hasBandpassSolution( String[] args )
  {
    id = getLongParam( args ) ;
    return proxy.hasBandpassSolution( id ) ? 1 : 0 ;
  }


  /**
   * Retrieves last issued solution ID.
   * @return last issued solution ID
   * @throws UnknownSolutionIdException 
   */
  private long getLatestSolutionID() throws UnknownSolutionIdException
  {
    return proxy.getLatestSolutionID() ;
  }


  /**
   * Gets smallest solution ID corresponding to the time >= the given timestamp.
   * Expects timestamp in the second program argument.
   * @param args - original program arguments
   * @return retrieved solution ID
   * @throws UnknownSolutionIdException if no such solution exists
   */
  private long getUpperBoundID( String[] args ) throws UnknownSolutionIdException
  {
    double time = getDoubleParam( args ) ;
    return proxy.getUpperBoundID( time ) ;
  }


  /**
   * Gets largest solution ID corresponding to the time <= the given timestamp
   * Expects timestamp in the second program argument.
   * @param args - original program arguments
   * @return retrieved solution ID
   * @throws UnknownSolutionIdException if no such solution exists
   */
  private long getLowerBoundID( String[] args ) throws UnknownSolutionIdException
  {
    double time = getDoubleParam( args ) ;
    return proxy.getLowerBoundID( time ) ;
  }


  /**
   * Gets gain solution with given ID.
   * Expects solution ID in the second argument and name of file to write the
   * solution to in the third argument.
   * 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if fails
   */
  private int getGainSolution( String[] args ) throws Exception
  {
    getLongAndStream( args ) ;
    object = proxy.getGainSolution( id ) ;
    writeObject() ;
    return 0;
  }


  /**
   * Gets leakage solution with given ID.
   * Expects solution ID in the second argument and name of file to write the
   * solution to in the third argument.
   * 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if fails
   */
  private int getLeakageSolution( String[] args ) throws Exception
  {
    getLongAndStream( args ) ;
    object = proxy.getLeakageSolution( id ) ;
    writeObject() ;
    return 0;
  }


  /**
   * Gets bandpass solution with given ID.
   * Expects solution ID in the second argument and name of file to write the
   * solution to in the third argument.
   * 
   * @param args - original program arguments
   * @return 0 if success
   * @throws Exception if fails
   */
  private int getBandpassSolution( String[] args ) throws Exception
  {
    getLongAndStream( args ) ;
    object = proxy.getBandpassSolution( id ) ;
    writeObject() ;
    return 0;
  }

  /**
   * Gets value of the second program argument as double.
   * @param args - original program args
   * @return value of the second program argument as double
   */
  private double getDoubleParam( String[] args )
  {
    return Double.parseDouble( args[ 2 ] ) ;
  }
 
  
  /**
   * Gets value of the second program argument as long.
   * @param args - original program args
   * @return value of the second program argument as long
   */
  private long getLongParam( String[] args )
  {
    return Long.parseLong( args[ 2 ] ) ;
  }
 
  
  /**
   * Gets value of the second program argument as long and reads a 
   * solution object from file identified by the third program argument.
   * Places these into class variables id and object.
   * @param args - original program args
   * @throws IOException 
   */
  private void getLongAndObject( String[] args ) throws Exception
  {
    id = Long.parseLong( args[ 2 ] ) ;
    
    try ( ObjectInput in = new ObjectInputStream( new FileInputStream( args[ 3 ] ) ) )
      {
        object = (Serializable)in.readObject() ; 
      } 
  }
 

  /**
   * Writes the object to the object stream and closes it.
   * @throws Exception if there are any problems
   */
  private void writeObject() throws Exception
  {
    out.writeObject( object ) ;
    out.close() ;
  }
  
  
  /**
   * Gets value of the second program argument as long and opens a
   * file identified by the third program argument for object output.
   * Places these into class variables id and stream.
   * @param args - original program args
   * @throws IOException 
   */
  private void getLongAndStream( String[] args ) throws Exception
  {
    id = Long.parseLong( args[ 2 ] ) ;
    out = new ObjectOutputStream( new FileOutputStream( args[ 3 ] ) ) ; 
  }

}
