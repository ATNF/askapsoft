package askap.services.caldataservice;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import askap.interfaces.caldataservice.AlreadyExists;
import askap.interfaces.caldataservice.UnknownSolutionIdException;
import askap.interfaces.calparams.TimeTaggedBandpassSolution;
import askap.interfaces.calparams.TimeTaggedGainSolution;
import askap.interfaces.calparams.TimeTaggedLeakageSolution;

/**
 * 
 * 
 * Copyright 2017, CSIRO Australia
 * All rights reserved.
 * 
 */
public class MySQLStorageTest
{
  static MySQLStorage storage ;
  static String tempDir ;
  static TimeTaggedBandpassSolution bSolution1, bSolution2 ;
  static TimeTaggedGainSolution gSolution3, gSolution4 ;
  static TimeTaggedLeakageSolution lSolution5, lSolution6 ;
  static long id1, id2, id3, id4, id5 ;

  /**
   * @throws java.lang.Exception
   */
  @BeforeClass
  public static void setUpBeforeClass() throws Exception
  {
    bSolution1 = Utils.makeBandpassSolution( 1 ) ;
    bSolution2 = Utils.makeBandpassSolution( 2 ) ;
    gSolution3 = Utils.makeGainSolution( 3 ) ;
    gSolution4 = Utils.makeGainSolution( 4 ) ;
    lSolution5 = Utils.makeLeakageSolution( 5 ) ;
    lSolution6 = Utils.makeLeakageSolution( 6 ) ;

    String dbUrl = "jdbc:h2:mem:testdb;DB_CLOSE_ON_EXIT=FALSE;MODE=MSSQLServer" ;
    String dbDriver = "org.h2.Driver" ;
    String dbUser = null ;
    String dbPassword = null ;
    
    Path temp = Files.createTempDirectory( "temp" ) ;
    tempDir = temp.toString() ;

    storage = new MySQLStorage( dbUrl, dbDriver, dbUser, dbPassword, tempDir ) ;
    id1 = storage.newSolutionID() ;
    id2 = storage.newSolutionID() ;
    id3 = storage.newSolutionID() ;
    id4 = storage.newSolutionID() ;
    id5 = storage.newSolutionID() ;
    
    storage.addBandpassSolution( id1, bSolution1 ) ;
    storage.addBandpassSolution( id2, bSolution2 ) ;
    storage.addBandpassSolution( id3, bSolution1 ) ;
    storage.addGainsSolution( id1, gSolution3 ) ;
    storage.addGainsSolution( id2, gSolution4 ) ;
    storage.addGainsSolution( id4, gSolution4 ) ;
    storage.addLeakageSolution( id1, lSolution5 ) ;
    storage.addLeakageSolution( id2, lSolution6 ) ;
    storage.addLeakageSolution( id5, lSolution6 ) ;

  }


  /**
   * @throws java.lang.Exception
   */
  @AfterClass
  public static void tearDownAfterClass() throws Exception
  {
    if ( storage != null ) try { storage.reset() ; storage.close() ; } catch( Exception e ) {};
    File toDelete = new File( tempDir ) ;
    Utils.deleteDirectory( toDelete.getAbsolutePath() ) ;    
  }

  /**
   * @throws java.lang.Exception
   */
  @Before
  public void setUp() throws Exception
  {
  }
  

  /**
   * @throws java.lang.Exception
   */
  @After
  public void tearDown() throws Exception
  {
  }

  
  @Test
  public void newSolutionIDTest()
  {
    long id = storage.newSolutionID() ;
    assertEquals( "Expected issued id is 7", id, 7 ) ;
    id = storage.newSolutionID() ;
    assertEquals( "Expected issued id is 8", id, 8 ) ;
  }

  
  @Test
  public void addBandpassSolutionTest() throws AlreadyExists, UnknownSolutionIdException
  {
    TimeTaggedBandpassSolution out = storage.getBandpassSolution( id1 ) ;
    boolean eq = Utils.equal( bSolution1, out ) ;
    
    assertEquals( "Written and read bandpass solutions are not the same", eq, true ) ;
    
    try
    {
      storage.addBandpassSolution( id1, bSolution1 ) ;
      fail( "Repeated adding of bandpass solution does not cause an AlreadyExists exception." );
    } 
    catch( AlreadyExists e ) {} 
  }


  @Test
  public void addGainsSolutionTest() throws AlreadyExists, UnknownSolutionIdException
  {
    TimeTaggedGainSolution out = storage.getGainSolution( id1 ) ;
    boolean eq = Utils.equal( gSolution3, out ) ;
    
    assertEquals( "Written and read gains solutions are not the same", eq, true ) ;
    
    try
    {
      storage.addGainsSolution( id1, gSolution3 ) ;
      fail( "Repeated adding of gains solution does not cause an AlreadyExists exception." );
    } 
    catch( AlreadyExists e ) {} 
  }

  
  @Test
  public void addLeakageSolutionTest() throws AlreadyExists, UnknownSolutionIdException
  {
    TimeTaggedLeakageSolution out = storage.getLeakageSolution( id1 ) ;
    boolean eq = Utils.equal( lSolution5, out ) ;
    
    assertEquals( "Written and read leakage solutions are not the same", eq, true ) ;
    
    try
    {
      storage.addLeakageSolution( id1, lSolution5 ) ;
      fail( "Repeated adding of leakage solution does not cause an AlreadyExists exception." );
    } 
    catch( AlreadyExists e ) {} 
  }
  
  

  @Test
  public void getGainsSolutionTest() throws AlreadyExists, UnknownSolutionIdException
  {
    addGainsSolutionTest() ;
    try
    {
      storage.getGainSolution( 999 ) ;
      fail( "Getting non-existing gains solution does not cause UnknownSolutionIdException." ) ;
    }
    catch( UnknownSolutionIdException e ) {} 
  }

  
  @Test
  public void getBandpassSolutionTest() throws AlreadyExists, UnknownSolutionIdException
  {
    addBandpassSolutionTest() ;
    try
    {
      storage.getBandpassSolution( 999 ) ;
      fail( "Getting non-existing  bandpass solution does not cause UnknownSolutionIdException." ) ;
    }
    catch( UnknownSolutionIdException e ) {} 
  }

  
  @Test
  public void getLeakageSolutionTest() throws AlreadyExists, UnknownSolutionIdException
  {
    addLeakageSolutionTest() ;
    try
    {
      storage.getLeakageSolution( 999 ) ;
      fail( "Getting non-existing leakage solution does not cause UnknownSolutionIdException." ) ;
    }
    catch( UnknownSolutionIdException e ) {} 
  }
  
  
  @Test
  public void hasBandpassSolutionTest()
  {
    boolean has = storage.hasBandpassSolution( id1 ) ;
    boolean hasnot = storage.hasBandpassSolution( id4 ) ;
    assertEquals( "Bandpass solution does exist", has, true ) ;
    assertEquals( "Bandpass solution does not exist", hasnot, false ) ;
  }
  
  
  @Test
  public void hasGainsSolutionTest()
  {
    boolean has = storage.hasGainSolution( id1 ) ;
    boolean hasnot = storage.hasGainSolution( id3 ) ;
    assertEquals( "Gains solution does exist for id = " + id1, has, true ) ;
    assertEquals( "Gains solution does not exist for id = " + id3, hasnot, false ) ;
  }
  
  
  @Test
  public void hasLeakageSolutionTest()
  {
    boolean has = storage.hasLeakageSolution( id1 ) ;
    boolean hasnot = storage.hasLeakageSolution( id4 ) ;
    assertEquals( "Leakage solution does exist for id = " + id1, has, true ) ;
    assertEquals( "Leakage solution does not exist for id = " + id4, hasnot, false ) ;
  }
  
  @Test
  public void getLatestSolutionIDTest() throws UnknownSolutionIdException
  {
    long id1 = storage.newSolutionID() ;
    long id2 = storage.getLatestSolutionID() ; 
    assertEquals( "Wrong latest solution id: " + id2 + ", must be " + id1, id1, id2 ) ;
  }
  
  
  @Test
  public void getUpperBoundIDTest() throws UnknownSolutionIdException
  {
    long id = storage.getUpperBoundID( 3.5 ) ;
    assertEquals( "Wrong upper bound solution id for time = 3.5: " + id + ", must be " + id2, id2, id ) ;
    
    id = storage.getUpperBoundID( 5.5 ) ;
    assertEquals( "Wrong upper bound solution id for time = 5.5: " + id + ", must be " + id2, id2, id ) ;
  }
  
  
  @Test
  public void getLowerBoundIDTest() throws UnknownSolutionIdException
  {
    long id = storage.getLowerBoundID( 1.5 ) ;
    assertEquals( "Wrong lower bound solution id for time = 1.5: " + id + ", must be " + id3, id3, id ) ;
    
    id = storage.getLowerBoundID( 5.5 ) ;
    assertEquals( "Wrong lower bound solution id for time = 5.5: " + id + ", must be " + id1, id1, id ) ;
  }
  
  
 
}
