package askap.services.caldataservice;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInput;
import java.io.ObjectInputStream;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;
import java.io.RandomAccessFile;
import java.io.Serializable;
import java.util.logging.Logger;
import java.util.zip.GZIPInputStream;


/**
 * 
 * 
 * Copyright 2017, CSIRO Australia
 * All rights reserved.
 * 
 */


public class MatrixStorage
{
  protected static final Logger LOG = Logger.getLogger(MatrixStorage.class.getName());

  
  /** Maximum chunk file size */
  static private long maxFileSize = 1024L * 1024 * 1024 * 2 ; // 2GB
  
  /** File name format */
  static final private String FILE_NAME_FORMAT = "%s/data%05d.bin" ;
  
  /** Directory with data files */
  private String baseDir ;
  
  /** Current file name */
  private String curFile ;
  
  /** Current R/W file handle */
  RandomAccessFile fh ;
  
  /** Last R/O file handle */ 
  DataInputStream dataReader ;
  
  /** Position in the read file */ 
  long readerPosition ;
  
  /** Current file number */
  private int curNum ;
  
  
  /**
   * 
   */
  public MatrixStorage( String baseDir, boolean clean ) throws Exception
  {
    File dirFile = new File( baseDir ) ;
    if ( !dirFile.exists() ) 
               dirFile.mkdirs() ;
    File[] files = dirFile.listFiles() ;
    
    this.baseDir = baseDir ;
    // find the last file number
    curNum = 1 ;
    for ( File file : files )
    {
      String name = file.getName() ;
      if ( !name.startsWith( "data" ) || !name.contains( ".bin" ) ) 
        continue ;
      if ( clean )
        { file.delete() ; continue ; }
      String numStr = name.substring( 4, name.indexOf( ".bin" ) ) ;
      try {
            int num = Integer.parseInt( numStr ) ;
            if ( num > curNum ) curNum = num ;
          } catch( Throwable e )
      {
        LOG.warning( "Can't read file number, ignoring file " + name ) ;   
      }
    }
    
    curFile = String.format( FILE_NAME_FORMAT, baseDir, curNum ) ;    
    fh = new RandomAccessFile( curFile, "rwd" ) ;
    dataReader = null ;
  }
  
  
  public synchronized long[] writeObject( Serializable object ) throws Exception
  {
    ByteArrayOutputStream bos = new ByteArrayOutputStream() ;
    ObjectOutput out = null ;
    try
    {
      out = new ObjectOutputStream( bos ) ;   
      out.writeObject( object ) ;
      out.flush() ;
      byte[] bytes = bos.toByteArray() ;
      return write( bytes ) ;
    } finally
    { 
      try { out.close() ; } catch ( IOException ex ) {} ;
    }
  }
  
  
  public synchronized Object readObject( int fileNum, long l, int length ) throws Exception
  {
    byte[] arr = read( fileNum, l, length, true ) ;
    
    ByteArrayInputStream bis = new ByteArrayInputStream( arr ) ;
    ObjectInput in = null ;
    try
    {
      in = new ObjectInputStream( bis ) ;
      Object o = in.readObject() ; 
      return o ;
    } finally
    {
      try { if (in != null) { in.close(); } } catch (IOException ex) {}
    }
  }

  
  public synchronized Object[] readObjects( int[] addresses ) throws Exception
  {
    byte[][] arr = read( addresses ) ;
    int len = addresses.length / 3 ;
    Object[] result = new Object[ len ] ; 
    
    for ( int i = 0 ; i < len ; i ++ )
    {
      ByteArrayInputStream bis = new ByteArrayInputStream( arr[ i ] ) ;
      ObjectInput in = null ;
      try
      {
        in = new ObjectInputStream( bis ) ;
        Object o = in.readObject() ; 
        result[ i ] = o ;
      } finally
      {
        try { if (in != null) { in.close(); } } catch (IOException ex) {}
      }
    }
    return result ;
  }

  
  public synchronized long[] write( byte[] arr ) throws Exception
  {
    
    long pos = fh.getFilePointer() ;
    long len = fh.length() ;
    if ( pos != len )
               { fh.seek( len ) ; pos = len ; }
    
    int size = arr.length ;
    if ( size + len > maxFileSize )
    {
      fh.close() ;
      curNum++ ;
      curFile = String.format( FILE_NAME_FORMAT, baseDir, curNum ) ;    
      fh = new RandomAccessFile( curFile, "rwd" ) ;
      len = 0 ;
    }
    
    fh.write( arr );
    
    return new long[] { curNum, pos, size } ;
  }
  
  
  public synchronized void rollBack( long address ) throws Exception
  {
    fh.setLength( address );
  }
  

  public synchronized byte[] read( int fileNum, long l, int length ) throws Exception
  {
    return read( fileNum, l, length, true ) ;
  }
  
  
  /**
   * Reads a number of byte arrays from places defined in the addr array. Avoids 
   * repetitive reopening of the same file, if possible
   * 
   * @param addr - addresses of byte arrays to read in triplets <file num, offset, length>
   * @return array of read byte arrays
   * @throws Exception if failed to read 
   */
  public synchronized byte[][] read( int [] addr ) throws Exception
  {
    byte[][] result = new byte[ addr.length / 3 ][] ;
    
    try 
    {
      for ( int i = 0, j = 0 ; i < addr.length ; i+= 3, j++ )
      {
        int fileNum = addr[ i + 0 ] ;
        int position = addr[ i + 1 ] ;
        int length = addr[ i + 2 ] ;
        // Is this the same file as before?
        byte[] arr = null ;
        if ( i > 0 && fileNum == addr[ i - 3 ] && fileNum != curNum 
             && position >= addr[ i - 3 + 1 ] + addr[ i - 3 + 2 ] )
                          arr = read( dataReader, position, length ) ;
        else
        {
          if ( dataReader != null ) dataReader.close() ;
          arr = read( fileNum, position, length, false ) ;
        }
        result[ j ] = arr ;
      }
    } finally
    {
      try { if ( dataReader != null ) dataReader.close() ; } catch( Exception e ) {}
      dataReader = null ;
    }
    
    return result ;
    
  }
  
  public synchronized byte[] read( int fileNum, long l, int length, boolean close ) throws Exception
  {
    byte[] result = null ;
    try
    {
      if ( fileNum == curNum ) return readCurrent( l, length ) ;
      String fileName = String.format( FILE_NAME_FORMAT, baseDir, fileNum ) ;
      if ( new File( fileName ).exists() ) result = readFile( fileName, l, length ) ;
      else
      {
        fileName += ".gz" ;
        if ( new File( fileName ).exists() ) result = readFile( fileName, l, length ) ;
        else throw new Exception( "File to read does not exist for number " + fileNum ) ;
      }
    }
    finally
    {
      if ( dataReader != null && close )
        try { dataReader.close(); } catch( Exception e ) {} finally { dataReader = null ; }
    }
    return result ;
  }
  
  
  /**
   * Reads an array of bytes from the currently written file
   * @param l
   * @param length
   * @return
   * @throws Exception 
   */
  private byte[] readCurrent( long l, int length ) throws Exception
  {
    fh.seek( l ) ;
    byte[] bytes = new byte[ length ] ;

    if ( fh.read( bytes ) != bytes.length ) 
      throw new Exception( "Could not read " + length + " bytes." ) ;

    return bytes ;
  }


  /**
   * @param fileName
   * @param l
   * @param length
   * @return
   * @throws Exception 
   */
  private byte[] readFile( String fileName, long l, int length ) throws Exception
  {
    dataReader = getDataInputStream( fileName ) ;
    readerPosition = 0 ;
    return read( dataReader, l, length ) ;
  }


  /**
   * @param fh2
   * @param position
   * @param length
   * @return
   * @throws Exception 
   */
  private byte[] read( DataInputStream reader, long position, int length ) throws Exception
  {
    if ( readerPosition > position )
      throw new Exception( "Can't move back in DataInputStream." ) ;
    if ( readerPosition < position ) 
      { reader.skip( position - readerPosition ) ; readerPosition = position ; }

    byte[] bytes = new byte[ length ] ;

    if ( reader.read( bytes ) != bytes.length ) 
      throw new Exception( "Could not read " + length + " bytes." ) ;
    readerPosition += bytes.length ; 
    
    return bytes ;
  }
  
  
  public static DataInputStream getDataInputStream( String fileName ) throws Exception
  {
      File file = new File( fileName ) ;
      InputStream fileStream = new FileInputStream( file.getAbsolutePath() ) ;
      DataInputStream inputStream = null ;
      if ( fileName.endsWith( ".gz" ) )
      {
        InputStream gzipStream = new GZIPInputStream( fileStream ) ;
        inputStream = new DataInputStream( gzipStream ) ;
      }
      else inputStream = new DataInputStream( fileStream ) ;

      return inputStream ;
  }


  public static long getMaxFileSize()
  {
    return maxFileSize;
  }


  public static void setMaxFileSize( long maxFileSize )
  {
    MatrixStorage.maxFileSize = maxFileSize ;
  }


  public void close() 
  {
    try { if ( dataReader != null ) dataReader.close() ; } catch( Exception e ) {}
    try { if ( fh != null ) fh.close() ; } catch( Exception e ) {}
    dataReader = null ;
    fh = null ;
  }
  
    
  public static void writeMatrixFile( String fileName, byte[] matrix ) throws Exception
  {
    try ( FileOutputStream os = new FileOutputStream( fileName  ) )
    {
      os.write( matrix ) ;
    }
  }

  
  
}
