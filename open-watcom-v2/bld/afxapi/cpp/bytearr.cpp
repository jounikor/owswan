/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2004-2013 The Open Watcom Contributors. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Implementation of CByteArray.
*
****************************************************************************/


#include "stdafx.h"

IMPLEMENT_SERIAL( CByteArray, CObject, 0 )

CByteArray::CByteArray()
/**********************/
{
    m_pData = NULL;
    m_nSize = 0;
    m_nMaxSize = 0;
    m_nGrowBy = 1;
}

CByteArray::~CByteArray()
/***********************/
{
    RemoveAll();
    FreeExtra();
}

void CByteArray::Serialize( CArchive &ar )
/****************************************/
{
    CObject::Serialize( ar );

    if( ar.IsStoring() ) {
        ar.WriteCount( m_nSize );
        for( int i = 0; i < m_nSize; i++ ) {
            ar << m_pData[i];
        }
    } else {
        int nSize = ar.ReadCount();
        SetSize( nSize );
        for( int i = 0; i < nSize; i++ ) {
            ar >> m_pData[i];
        }
    }
}

#ifdef _DEBUG

void CByteArray::AssertValid() const
/**********************************/
{
    CObject::AssertValid();
    
    ASSERT( m_nSize >= 0 );
}

void CByteArray::Dump( CDumpContext &dc ) const
/*********************************************/
{
    CObject::Dump( dc );

    dc << "m_pData = " << m_pData << "\n";
    dc << "m_nSize = " << m_nSize << "\n";
    dc << "m_nMaxSize = " << m_nMaxSize << "\n";
    dc << "m_nGrowBy = " << m_nGrowBy << "\n";
}

#endif // _DEBUG

INT_PTR CByteArray::Add( BYTE newElement )
/****************************************/
{
    if( m_nSize == m_nMaxSize ) {
        m_nMaxSize += m_nGrowBy;
        BYTE *pNewData = new BYTE[m_nMaxSize];
        if( m_pData != NULL ) {
            memcpy( pNewData, m_pData, m_nSize * sizeof( BYTE ) );
            delete [] m_pData;
        }
        if( m_nGrowBy > 1 ) {
            memset( pNewData + m_nSize + 1, 0, (m_nGrowBy - 1) * sizeof( BYTE ) );
        }
        m_pData = pNewData;
    }
    m_pData[m_nSize] = newElement;
    int nIndex = m_nSize;
    m_nSize++;
    return( nIndex );
}

INT_PTR CByteArray::Append( const CByteArray &src )
/*************************************************/
{
    int nNewMaxSize = m_nSize + src.m_nSize;
    if( nNewMaxSize > m_nMaxSize ) {
        if( nNewMaxSize % m_nGrowBy != 0 ) {
            nNewMaxSize -= nNewMaxSize % m_nGrowBy;
            nNewMaxSize += m_nGrowBy;
        }
        m_nMaxSize = nNewMaxSize;
        BYTE *pNewData = new BYTE[m_nMaxSize];
        if( m_pData != NULL ) {
            memcpy( pNewData, m_pData, m_nSize * sizeof( BYTE ) );
            delete [] m_pData;
        }
        if( m_nMaxSize > m_nSize + src.m_nSize ) {
            memset( pNewData + m_nSize + src.m_nSize, 0,
                    (m_nMaxSize - (m_nSize + src.m_nSize)) * sizeof( BYTE ) );
        }
        m_pData = pNewData;
    }
    memcpy( m_pData + m_nSize, src.m_pData, src.m_nSize * sizeof( BYTE ) );
    int nIndex = m_nSize;
    m_nSize += src.m_nSize;
    return( nIndex );
}

void CByteArray::Copy( const CByteArray &src )
/********************************************/
{
    int nNewMaxSize = src.m_nSize;
    if( nNewMaxSize > m_nMaxSize ) {
        if( nNewMaxSize % m_nGrowBy != 0 ) {
            nNewMaxSize -= nNewMaxSize % m_nGrowBy;
            nNewMaxSize += m_nGrowBy;
        }
        m_nMaxSize = nNewMaxSize;
        if( m_pData != NULL ) {
            delete [] m_pData;
        }
        m_pData = new BYTE[m_nMaxSize];
        if( m_nMaxSize > src.m_nSize ) {
            memset( m_pData + src.m_nSize, 0,
                    (m_nMaxSize - src.m_nSize) * sizeof( BYTE ) );
        }
    }
    memcpy( m_pData, src.m_pData, src.m_nSize * sizeof( BYTE ) );
    m_nSize = src.m_nSize;
}

void CByteArray::FreeExtra()
/**************************/
{
    if( m_nMaxSize > m_nSize && m_pData != NULL ) {
        if( m_nSize > 0 ) {
            BYTE *pNewData = new BYTE[m_nSize];
            memcpy( pNewData, m_pData, m_nSize * sizeof( BYTE ) );
            delete [] m_pData;
            m_pData = pNewData;
        } else {
            delete [] m_pData;
            m_pData = NULL;
        }
        m_nMaxSize = m_nSize;
    }
}

void CByteArray::InsertAt( INT_PTR nIndex, BYTE newElement, int nCount )
/**********************************************************************/
{
    ASSERT( nIndex >= 0 );
    ASSERT( nCount >= 1 );
    int nNewMaxSize = m_nSize + nCount;
    if( nNewMaxSize > m_nMaxSize ) {
        if( nNewMaxSize % m_nGrowBy != 0 ) {
            nNewMaxSize -= nNewMaxSize % m_nGrowBy;
            nNewMaxSize += m_nGrowBy;
        }
        m_nMaxSize = nNewMaxSize;
        BYTE *pNewData = new BYTE[m_nMaxSize];
        if( m_nSize + nCount < m_nMaxSize ) {
            memset( pNewData + m_nSize + nCount, 0,
                    (m_nMaxSize - (m_nSize + nCount)) * sizeof( BYTE ) );
        }
        if( m_pData != NULL ) {
            memcpy( pNewData, m_pData, m_nSize * sizeof( BYTE ) );
            delete [] m_pData;
        }
        m_pData = pNewData;
    }
    memmove( m_pData + nIndex + nCount, m_pData + nIndex,
             (m_nSize - nIndex) * sizeof( BYTE ) );
    for( int i = 0; i < nCount; i++ ) {
        m_pData[nIndex + i] = newElement;
    }
    m_nSize += nCount;
}

void CByteArray::InsertAt( INT_PTR nStartIndex, CByteArray *pNewArray )
/*********************************************************************/
{
    ASSERT( nStartIndex >= 0 );
    ASSERT( pNewArray != NULL );
    int nNewMaxSize = m_nSize + pNewArray->m_nSize;
    if( nNewMaxSize > m_nMaxSize ) {
        if( nNewMaxSize % m_nGrowBy != 0 ) {
            nNewMaxSize -= nNewMaxSize % m_nGrowBy;
            nNewMaxSize += m_nGrowBy;
        }
        m_nMaxSize = nNewMaxSize;
        BYTE *pNewData = new BYTE[m_nMaxSize];
        if( m_pData != NULL ) {
            memcpy( pNewData, m_pData, m_nSize * sizeof( BYTE ) );
            delete [] m_pData;
        }
        if( m_nSize + pNewArray->m_nSize < m_nMaxSize ) {
            memset( pNewData + m_nSize + pNewArray->m_nSize, 0,
                    (m_nMaxSize - (m_nSize + pNewArray->m_nSize)) * sizeof( BYTE ) );
        }
        m_pData = pNewData;
    }
    memmove( m_pData + nStartIndex + pNewArray->m_nSize, m_pData + nStartIndex,
             (m_nSize - nStartIndex) * sizeof( BYTE ) );
    memcpy( m_pData + nStartIndex, pNewArray->m_pData,
            pNewArray->m_nSize * sizeof( BYTE ) );
    m_nSize += pNewArray->m_nSize;
}

void CByteArray::RemoveAt( INT_PTR nIndex, INT_PTR nCount )
/*********************************************************/
{
    ASSERT( nCount >= 1 );
    ASSERT( nIndex + nCount <= m_nSize );
    if( nIndex + nCount < m_nSize ) {
        memmove( m_pData + nIndex, m_pData + nIndex + nCount,
                 (m_nSize - (nIndex + nCount)) * sizeof( BYTE ) );
    }
    m_nSize -= nCount;
    memset( m_pData + m_nSize, 0, nCount * sizeof( BYTE ) );
}

void CByteArray::SetAtGrow( INT_PTR nIndex, BYTE newElement )
/***********************************************************/
{
    ASSERT( nIndex >= 0 );
    int nNewMaxSize = nIndex + 1;
    if( nNewMaxSize > m_nMaxSize ) {
        if( nNewMaxSize % m_nGrowBy != 0 ) {
            nNewMaxSize -= nNewMaxSize % m_nGrowBy;
            nNewMaxSize += m_nGrowBy;
        }
        m_nMaxSize = nNewMaxSize;
        BYTE *pNewData = new BYTE[m_nMaxSize];
        if( m_pData != NULL ) {
            memcpy( pNewData, m_pData, m_nSize * sizeof( BYTE ) );
            delete [] m_pData;
        }
        ASSERT( m_nSize < m_nMaxSize );
        memset( pNewData + m_nSize, 0, (m_nMaxSize - m_nSize) * sizeof( BYTE ) );
        m_pData = pNewData;
    }
    if( nIndex >= m_nSize ) {
        m_nSize = nIndex + 1;
    }
    m_pData[nIndex] = newElement;
}

void CByteArray::SetSize( INT_PTR nNewSize, INT_PTR nGrowBy )
/***********************************************************/
{
    ASSERT( nNewSize >= 0 );
    if( nGrowBy > 0 ) {
        m_nGrowBy = nGrowBy;
    }
    int nNewMaxSize = nNewSize;
    if( nNewMaxSize > m_nMaxSize ) {
        if( nNewMaxSize % m_nGrowBy != 0 ) {
            nNewMaxSize -= nNewMaxSize % m_nGrowBy;
            nNewMaxSize += m_nGrowBy;
        }
        m_nMaxSize = nNewMaxSize;
        BYTE *pNewData = new BYTE[m_nMaxSize];
        if( m_pData != NULL ) {
            memcpy( pNewData, m_pData, m_nSize * sizeof( BYTE ) );
            delete [] m_pData;
        }
        if( m_nSize < m_nMaxSize ) {
            memset( pNewData + m_nSize, 0, (m_nMaxSize - m_nSize) * sizeof( BYTE ) );
        }
        m_pData = pNewData;
    }
    m_nSize = nNewSize;
    FreeExtra();
}
