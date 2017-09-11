module Globber (matchGlob) where

type GlobPattern = String

matchGlob :: GlobPattern -> String -> Bool
matchGlob _ _ = False

