-- \set owner OWNER
-- create database manually
-- CREATE DATABASE wendzelnntpd owner :owner;

-- Used, when setting ‘message-count-in-db’ is active:
CREATE SEQUENCE nntp_next_msg_id INCREMENT BY 1;

-- MsgID-Len is always 196 here
-- NG-Name-Len is always 196

CREATE TABLE newsgroups (
   id SERIAL PRIMARY KEY ,
   "name" VARCHAR(196) unique,
   pflag CHARACTER,
   high INTEGER
 );

CREATE TABLE postings (
   msgid VARCHAR(196) PRIMARY KEY,
   "date" INTEGER,
   author VARCHAR(196),
   newsgroups VARCHAR(2048),
   subject VARCHAR(2048),
   lines VARCHAR(10),
   header VARCHAR(20000)
 );

-- Used, when setting ‘message-body-in-db’ is active:
-- Index and tsvector() creation must be specified manually
-- because it depends on the language(s) which are used:
-- See also: https://www.postgresql.org/docs/current/textsearch.html
CREATE TABLE body (
  msgid VARCHAR(196) PRIMARY KEY,
  content text
  -- Option 1: Add preprocessed column:
  -- , GENERATED ALWAYS AS (to_tsvector('english', event_narrative)
  -- Or by ALTER TABLE command:
  -- ALTER TABLE body ADD COLUMN ts tsvector
  --   GENERATED ALWAYS AS ((to_tsvector(‘english’), body)) STORED;

  -- Option 2: Add index:
  -- Consider also creating an index:
  --  CREATE INDEX ts_body_idx ON body USING GIN (to_tsvector('english', body));

);

CREATE TABLE ngposts (
   msgid VARCHAR(196) REFERENCES postings (msgid) ON DELETE CASCADE,
   ng VARCHAR(196) REFERENCES newsgroups ("name") ON DELETE CASCADE,
   postnum INTEGER,
   primary key (msgid, ng)
 );

-- Authentification (as well as ACL)
CREATE TABLE users (
   "name" VARCHAR(50) primary key,
   password VARCHAR(1024)
);

--
-- This is all needed for ACL
--

-- Define roles
CREATE TABLE roles (
   "role" VARCHAR(50) primary key
 );

-- User 2 Roles
CREATE TABLE users2roles (
   username VARCHAR(50) references users ("name") on delete cascade,
   "role" VARCHAR(50) references roles ("role") on delete cascade,
   PRIMARY KEY(username, "role")
 );

-- User 2 Newsgroup
CREATE TABLE acl_users (
   username VARCHAR(50) references users ("name") on delete cascade,
   ng VARCHAR(196) references newsgroups ("name") on delete cascade,
   PRIMARY KEY(username, ng)
 );

-- Roles 2 Newsgroup
CREATE TABLE acl_roles (
   "role" VARCHAR(50) references roles ("role") on delete cascade,
   ng VARCHAR(196) references newsgroups ("name") on delete cascade,
   PRIMARY KEY("role", ng)
 ) ;


CREATE OR REPLACE FUNCTION xhdr_get (
    hdr_type_str varchar,
    group_str varchar,
    msgid varchar,
    minval int default null,
    maxval int default null)
returns TABLE (
   postnum integer,
   req_col varchar)
as $$
DECLARE
   v_wherefilter varchar;
BEGIN
   if msgid is not null then
      v_wherefilter =
        format('where ng=''%s'' and p.msgid=''%s''', group_str, msgid);
   else
      v_wherefilter =
        format('where ng=''%s'' and (postnum between %s and %s)',
	group_str, minval, maxval);
   end if;

   return query execute
      format(
        $dsql$
         SELECT n.postnum, (p.%I)::varchar
          FROM ngposts n inner join postings p on (n.msgid = p.msgid)
          %s
        $dsql$, hdr_type_str, v_wherefilter);
END;
$$ LANGUAGE plpgsql VOLATILE SECURITY DEFINER;
